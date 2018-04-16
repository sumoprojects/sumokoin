// Copyright (c) 2017, SUMOKOIN
// Copyright (c) 2014-2017, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

/*!
 * \file simplewallet.cpp
 *
 * \brief Source file that defines simple_wallet class.
 */
#include <thread>
#include <iostream>
#include <sstream>
#include <fstream>
#include <ctype.h>
#include <math.h>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <boost/archive/portable_binary_oarchive.hpp>
#include <boost/archive/portable_binary_iarchive.hpp>

#include "include_base_utils.h"
#include "common/i18n.h"
#include "common/command_line.h"
#include "common/util.h"
#include "p2p/net_node.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "simplewallet.h"
#include "cryptonote_core/cryptonote_format_utils.h"
#include "storages/http_abstract_invoke.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "crypto/crypto.h"  // for crypto::secret_key definition
#include "mnemonics/electrum-words.h"
#include "rapidjson/document.h"
#include "common/json_util.h"
#include "ringct/rctSigs.h"
#include "wallet/wallet_args.h"
#include <stdexcept>

using namespace std;
using namespace epee;
using namespace cryptonote;
using boost::lexical_cast;
namespace po = boost::program_options;
typedef cryptonote::simple_wallet sw;

#define EXTENDED_LOGS_FILE "wallet_details.log"

#define KEY_IMAGE_EXPORT_FILE_MAGIC "Sumokoin key image export\002"
#define OUTPUT_EXPORT_FILE_MAGIC "Sumokoin output export\002"

#define LOCK_IDLE_SCOPE() \
  bool auto_refresh_enabled = m_auto_refresh_enabled.load(std::memory_order_relaxed); \
  m_auto_refresh_enabled.store(false, std::memory_order_relaxed); \
  /* stop any background refresh, and take over */ \
  m_wallet->stop(); \
  m_idle_mutex.lock(); \
  while (m_auto_refresh_refreshing) \
    m_idle_cond.notify_one(); \
  m_idle_mutex.unlock(); \
/*  if (auto_refresh_run)*/ \
    /*m_auto_refresh_thread.join();*/ \
  boost::unique_lock<boost::mutex> lock(m_idle_mutex); \
  epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){ \
    m_auto_refresh_enabled.store(auto_refresh_enabled, std::memory_order_relaxed); \
  })

enum TransferType {
  TransferOriginal,
  TransferNew,
  TransferLocked,
};

namespace
{
  const auto allowed_priority_strings = { "default", "unimportant", "normal", "elevated", "priority" };
  const auto arg_wallet_file = wallet_args::arg_wallet_file();
  const command_line::arg_descriptor<std::string> arg_generate_new_wallet = {"generate-new-wallet", sw::tr("Generate new wallet and save it to <arg>"), ""};
  const command_line::arg_descriptor<std::string> arg_generate_from_view_key = {"generate-from-view-key", sw::tr("Generate incoming-only wallet from view key"), ""};
  const command_line::arg_descriptor<std::string> arg_generate_from_keys = {"generate-from-keys", sw::tr("Generate wallet from private keys"), ""};
  const auto arg_generate_from_json = wallet_args::arg_generate_from_json();
  const command_line::arg_descriptor<std::string> arg_electrum_seed = {"electrum-seed", sw::tr("Specify Electrum seed for wallet recovery/creation"), ""};
  const command_line::arg_descriptor<bool> arg_restore_deterministic_wallet = {"restore-deterministic-wallet", sw::tr("Recover wallet using Electrum-style mnemonic seed"), false};
  const command_line::arg_descriptor<bool> arg_non_deterministic = {"non-deterministic", sw::tr("Create non-deterministic view and spend keys"), false};
  const command_line::arg_descriptor<bool> arg_trusted_daemon = {"trusted-daemon", sw::tr("Enable commands which rely on a trusted daemon"), false};
  const command_line::arg_descriptor<bool> arg_allow_mismatched_daemon_version = {"allow-mismatched-daemon-version", sw::tr("Allow communicating with a daemon that uses a different RPC version"), false};
  const command_line::arg_descriptor<uint64_t> arg_restore_height = {"restore-height", sw::tr("Restore from specific blockchain height"), 0};

  const command_line::arg_descriptor< std::vector<std::string> > arg_command = {"command", ""};

  inline std::string interpret_rpc_response(bool ok, const std::string& status)
  {
    std::string err;
    if (ok)
    {
      if (status == CORE_RPC_STATUS_BUSY)
      {
        err = sw::tr("daemon is busy. Please try again later.");
      }
      else if (status != CORE_RPC_STATUS_OK)
      {
        err = status;
      }
    }
    else
    {
      err = sw::tr("possibly lost connection to daemon");
    }
    return err;
  }

  class message_writer
  {
  public:
    message_writer(epee::log_space::console_colors color = epee::log_space::console_color_default, bool bright = false,
      std::string&& prefix = std::string(), int log_level = LOG_LEVEL_2)
      : m_flush(true)
      , m_color(color)
      , m_bright(bright)
      , m_log_level(log_level)
    {
      m_oss << prefix;
    }

    message_writer(message_writer&& rhs)
      : m_flush(std::move(rhs.m_flush))
#if defined(_MSC_VER)
      , m_oss(std::move(rhs.m_oss))
#else
      // GCC bug: http://gcc.gnu.org/bugzilla/show_bug.cgi?id=54316
      , m_oss(rhs.m_oss.str(), ios_base::out | ios_base::ate)
#endif
      , m_color(std::move(rhs.m_color))
      , m_log_level(std::move(rhs.m_log_level))
    {
      rhs.m_flush = false;
    }

    template<typename T>
    std::ostream& operator<<(const T& val)
    {
      m_oss << val;
      return m_oss;
    }

    ~message_writer()
    {
      if (m_flush)
      {
        m_flush = false;

        LOG_PRINT(m_oss.str(), m_log_level);

        if (epee::log_space::console_color_default == m_color)
        {
          std::cout << m_oss.str();
        }
        else
        {
          epee::log_space::set_console_color(m_color, m_bright);
          std::cout << m_oss.str();
          epee::log_space::reset_console_color();
        }
        std::cout << std::endl;
      }
    }

  private:
    message_writer(message_writer& rhs);
    message_writer& operator=(message_writer& rhs);
    message_writer& operator=(message_writer&& rhs);

  private:
    bool m_flush;
    std::stringstream m_oss;
    epee::log_space::console_colors m_color;
    bool m_bright;
    int m_log_level;
  };

  message_writer success_msg_writer(bool color = false)
  {
    return message_writer(color ? epee::log_space::console_color_green : epee::log_space::console_color_default, false, std::string(), LOG_LEVEL_2);
  }

  message_writer fail_msg_writer()
  {
    return message_writer(epee::log_space::console_color_red, true, sw::tr("Error: "), LOG_LEVEL_0);
  }

  bool is_it_true(const std::string& s)
  {
    if (s == "1" || command_line::is_yes(s))
      return true;

    boost::algorithm::is_iequal ignore_case{};
    if (boost::algorithm::equals("true", s, ignore_case))
      return true;
    if (boost::algorithm::equals(simple_wallet::tr("true"), s, ignore_case))
      return true;

    return false;
  }

  const struct
  {
    const char *name;
    tools::wallet2::RefreshType refresh_type;
  } refresh_type_names[] =
  {
    { "full", tools::wallet2::RefreshFull },
    { "optimize-coinbase", tools::wallet2::RefreshOptimizeCoinbase },
    { "optimized-coinbase", tools::wallet2::RefreshOptimizeCoinbase },
    { "no-coinbase", tools::wallet2::RefreshNoCoinbase },
    { "default", tools::wallet2::RefreshDefault },
  };

  bool parse_refresh_type(const std::string &s, tools::wallet2::RefreshType &refresh_type)
  {
    for (size_t n = 0; n < sizeof(refresh_type_names) / sizeof(refresh_type_names[0]); ++n)
    {
      if (s == refresh_type_names[n].name)
      {
        refresh_type = refresh_type_names[n].refresh_type;
        return true;
      }
    }
    fail_msg_writer() << cryptonote::simple_wallet::tr("failed to parse refresh type");
    return false;
  }

  std::string get_refresh_type_name(tools::wallet2::RefreshType type)
  {
    for (size_t n = 0; n < sizeof(refresh_type_names) / sizeof(refresh_type_names[0]); ++n)
    {
      if (type == refresh_type_names[n].refresh_type)
        return refresh_type_names[n].name;
    }
    return "invalid";
  }

  std::string get_version_string(uint32_t version)
  {
    return boost::lexical_cast<std::string>(version >> 16) + "." + boost::lexical_cast<std::string>(version & 0xffff);
  }

  bool parse_subaddress_indices(const std::string& arg, std::set<uint32_t>& subaddr_indices)
  {
    subaddr_indices.clear();

    if (arg.substr(0, 6) != "index=")
      return false;
    std::string subaddr_indices_str_unsplit = arg.substr(6, arg.size() - 6);
    std::vector<std::string> subaddr_indices_str;
    boost::split(subaddr_indices_str, subaddr_indices_str_unsplit, boost::is_any_of(","));

    for (const auto& subaddr_index_str : subaddr_indices_str)
    {
      uint32_t subaddr_index;
      if (!epee::string_tools::get_xtype_from_string(subaddr_index, subaddr_index_str))
      {
        fail_msg_writer() << tr("failed to parse index: ") << subaddr_index_str;
        subaddr_indices.clear();
        return false;
      }
      subaddr_indices.insert(subaddr_index);
    }
    return true;
  }
}


std::string simple_wallet::get_commands_str()
{
  std::stringstream ss;
  ss << tr("Commands: ") << ENDL;
  std::string usage = m_cmd_binder.get_usage();
  boost::replace_all(usage, "\n", "\n  ");
  usage.insert(0, "  ");
  ss << usage << ENDL;
  return ss.str();
}

bool simple_wallet::viewkey(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if (!get_and_verify_password()) { return true; }
  // don't log
  std::cout << "secret: " << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_view_secret_key) << std::endl;
  std::cout << "public: " << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_account_address.m_view_public_key) << std::endl;

  return true;
}

bool simple_wallet::spendkey(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and has no spend key");
    return true;
  }
  if (!get_and_verify_password()) { return true; }
  // don't log
  std::cout << "secret: " << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_spend_secret_key) << std::endl;
  std::cout << "public: " << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_account_address.m_spend_public_key) << std::endl;

  return true;
}

bool simple_wallet::seed(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  bool success =  false;
  std::string electrum_words;

  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and has no seed");
    return true;
  }
  if (!get_and_verify_password()) { return true; }
  if (m_wallet->is_deterministic())
  {
    if (m_wallet->get_seed_language().empty())
    {
      std::string mnemonic_language = get_mnemonic_language();
      if (mnemonic_language.empty())
        return true;
      m_wallet->set_seed_language(mnemonic_language);
    }

    success = m_wallet->get_seed(electrum_words);
  }

  if (success)
  {
    print_seed(electrum_words);
  }
  else
  {
    fail_msg_writer() << tr("wallet is non-deterministic and has no seed");
  }
  return true;
}

bool simple_wallet::seed_set_language(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  bool success = false;
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and has no seed");
    return true;
  }
  if (!m_wallet->is_deterministic())
  {
    fail_msg_writer() << tr("wallet is non-deterministic and has no seed");
    return true;
  }

  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    std::string mnemonic_language = get_mnemonic_language();
    if (mnemonic_language.empty())
      return true;

    m_wallet->set_seed_language(std::move(mnemonic_language));
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
  }
  return true;
}

bool simple_wallet::set_always_confirm_transfers(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    m_wallet->always_confirm_transfers(is_it_true(args[1]));
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
  }
  return true;
}

bool simple_wallet::set_store_tx_info(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot transfer");
    return true;
  }

  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    m_wallet->store_tx_info(is_it_true(args[1]));
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
  }
  return true;
}

bool simple_wallet::set_default_mixin(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  bool success = false;
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot transfer");
    return true;
  }

  std::string failed_msg = (boost::format(tr("mixin must be an integer >= %s and <= %s")) % DEFAULT_MIXIN % MAX_MIXIN).str();
  try
  {
    if (strchr(args[1].c_str(), '-'))
    {
      fail_msg_writer() << failed_msg;
      return true;
    }
    uint32_t mixin = boost::lexical_cast<uint32_t>(args[1]);
    if (mixin != 0 && (mixin < DEFAULT_MIXIN || mixin > MAX_MIXIN))
    {
      fail_msg_writer() << failed_msg;
      return true;
    }
    if (mixin == 0)
      mixin = DEFAULT_MIXIN;

    const auto pwd_container = get_and_verify_password();
    if (pwd_container)
    {
      m_wallet->default_mixin(mixin);
      m_wallet->rewrite(m_wallet_file, pwd_container->password());
    }
    return true;
  }
  catch(const boost::bad_lexical_cast &)
  {
    fail_msg_writer() << failed_msg;
    return true;
  }
  catch(...)
  {
    fail_msg_writer() << tr("could not change default mixin");
    return true;
  }
}

bool simple_wallet::set_default_priority(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  bool success = false;
  int priority = 0;
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot transfer");
    return true;
  }
  try
  {
    if (strchr(args[1].c_str(), '-'))
    {
      fail_msg_writer() << tr("priority must be 0, 1, 2, 3, 4 or 5");
      return true;
    }
    if (args[1] == "0")
    {
      priority = 0;
    }
    else
    {
      priority = boost::lexical_cast<int>(args[1]);
      if (priority != 1 && priority != 2 && priority != 3 && priority != 4 && priority != 5)
      {
        fail_msg_writer() << tr("priority must be 0, 1, 2, 3, 4 or 5");
        return true;
      }
    }

    const auto pwd_container = get_and_verify_password();
    if (pwd_container)
    {
      m_wallet->set_default_priority(priority);
      m_wallet->rewrite(m_wallet_file, pwd_container->password());
    }
    return true;
  }
  catch(const boost::bad_lexical_cast &)
  {
    fail_msg_writer() << tr("priority must be 0, 1, 2, 3, 4 or 5");
    return true;
  }
  catch(...)
  {
    fail_msg_writer() << tr("could not change default priority");
    return true;
  }
}

bool simple_wallet::set_auto_refresh(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{

  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    const bool auto_refresh = is_it_true(args[1]);
    m_wallet->auto_refresh(auto_refresh);
    m_idle_mutex.lock();
    m_auto_refresh_enabled.store(auto_refresh, std::memory_order_relaxed);
    m_idle_cond.notify_one();
    m_idle_mutex.unlock();

    m_wallet->rewrite(m_wallet_file, pwd_container->password());
  }
  return true;
}

bool simple_wallet::set_refresh_type(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  tools::wallet2::RefreshType refresh_type;
  if (!parse_refresh_type(args[1], refresh_type))
  {
    return true;
  }

  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    m_wallet->set_refresh_type(refresh_type);
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
  }
  return true;
}

bool simple_wallet::set_confirm_missing_payment_id(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot transfer");
    return true;
  }

  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    m_wallet->confirm_missing_payment_id(is_it_true(args[1]));
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
  }
  return true;
}

bool simple_wallet::help(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  success_msg_writer() << get_commands_str();
  return true;
}

simple_wallet::simple_wallet()
  : m_allow_mismatched_daemon_version(false)
  , m_refresh_progress_reporter(*this)
  , m_idle_run(true)
  , m_auto_refresh_enabled(false)
  , m_auto_refresh_refreshing(false)
  , m_in_manual_refresh(false)
  , m_current_subaddress_account(0)
{
  m_cmd_binder.set_handler("start_mining", boost::bind(&simple_wallet::start_mining, this, _1), tr("start_mining [<number_of_threads>] - Start mining in daemon"));
  m_cmd_binder.set_handler("stop_mining", boost::bind(&simple_wallet::stop_mining, this, _1), tr("Stop mining in daemon"));
  m_cmd_binder.set_handler("save_bc", boost::bind(&simple_wallet::save_bc, this, _1), tr("Save current blockchain data"));
  m_cmd_binder.set_handler("refresh", boost::bind(&simple_wallet::refresh, this, _1), tr("Synchronize transactions and balance"));
  m_cmd_binder.set_handler("balance", boost::bind(&simple_wallet::show_balance, this, _1), tr("balance [detail] - Show wallet balance of currently selected account"));
  m_cmd_binder.set_handler("incoming_transfers", boost::bind(&simple_wallet::show_incoming_transfers, this, _1), tr("incoming_transfers [available|unavailable] [verbose] [index=<N1>[,<N2>,...]] - Show incoming transfers, all or filtered by availability and address index"));
  m_cmd_binder.set_handler("payments", boost::bind(&simple_wallet::show_payments, this, _1), tr("payments <PID_1> [<PID_2> ... <PID_N>] - Show payments for given payment ID[s]"));
  m_cmd_binder.set_handler("bc_height", boost::bind(&simple_wallet::show_blockchain_height, this, _1), tr("Show blockchain height"));
  m_cmd_binder.set_handler("transfer", boost::bind(&simple_wallet::transfer_new, this, _1), tr("transfer [index=<N1>[,<N2>,...]] [<priority>] [<mixin_count>] <address> <amount> [<payment_id>] - Transfer <amount> to <address>. If the parameter \"index=<N1>[,<N2>,...]\" is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. <priority> is the priority of the transaction. The higher the priority, the higher the fee of the transaction. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command \"set priority\") is used. <mixin_count> is the number of extra inputs to include for untraceability. Multiple payments can be made at once by adding <address_2> <amount_2> etcetera (before the payment ID, if it's included)"));
  m_cmd_binder.set_handler("locked_transfer", boost::bind(&simple_wallet::locked_transfer, this, _1), tr("locked_transfer [index=<N1>[,<N2>,...]] [<priority>] [<mixin_count>] <addr> <amount> <lockblocks> [<payment_id>] - Same as transfer, but with number of blocks to lock the transaction for, max 1000000"));
  m_cmd_binder.set_handler("sweep_all", boost::bind(&simple_wallet::sweep_all, this, _1), tr("sweep_all [index=<N1>[,<N2>,...]] [<mixin_count>] <address> [<payment_id>] - Send all unlocked balance to an address. If the parameter \"index<N1>[,<N2>,...]\" is specified, the wallet sweeps outputs received by those address indices. If omitted, the wallet randomly chooses an address index to be used."));
  m_cmd_binder.set_handler("sign_transfer", boost::bind(&simple_wallet::sign_transfer, this, _1), tr("Sign a transaction from a file"));
  m_cmd_binder.set_handler("submit_transfer", boost::bind(&simple_wallet::submit_transfer, this, _1), tr("Submit a signed transaction from a file"));
  m_cmd_binder.set_handler("set_log", boost::bind(&simple_wallet::set_log, this, _1), tr("set_log <level> - Change current log detail level, <0-4>"));
  m_cmd_binder.set_handler("account", boost::bind(&simple_wallet::account, this, _1), tr("account [new <label text with white spaces allowed> | switch <index> | label <index> <label text with white spaces allowed>] - If no argments are specified, the wallet shows all the existing accounts along with their balances. If the \"new\" argument is specified, the wallet creates a new account with its label initialized by the provided label text (which can be empty). If the \"switch\" argument is specified, the wallet switches to the account specified by <index>. If the \"label\" argument is specified, the wallet sets the label of the account specified by <index> to the provided label text."));
  m_cmd_binder.set_handler("address", boost::bind(&simple_wallet::print_address, this, _1), tr("address [ new <label text with white spaces allowed> | all | <index_min> [<index_max>] | label <index> <label text with white spaces allowed> ] - If no arguments are specified or <index> is specified, the wallet shows the specified address. If \"all\" is specified, the walllet shows all the existing addresses in the currently selected account. If \"new \" is specified, the wallet creates a new address with the provided label text (which can be empty). If \"label\" is specified, the wallet sets the label of the address specified by <index> to the provided label text."));
  m_cmd_binder.set_handler("integrated_address", boost::bind(&simple_wallet::print_integrated_address, this, _1), tr("integrated_address [<index>] [<payment_id>] - Encode <payment_id> into an integrated address for the wallet public address at <index>. If <index> is omitted, the wallet uses the base address (index=0). If <payment_id> is omitted, the wallet uses a random payment ID."));
  m_cmd_binder.set_handler("parse_address", boost::bind(&simple_wallet::parse_address, this, _1), tr("parse_address <addr> - Decode an address string and print detailed info"));
  m_cmd_binder.set_handler("save", boost::bind(&simple_wallet::save, this, _1), tr("Save wallet data"));
  m_cmd_binder.set_handler("save_watch_only", boost::bind(&simple_wallet::save_watch_only, this, _1), tr("Save a watch-only keys file"));
  m_cmd_binder.set_handler("viewkey", boost::bind(&simple_wallet::viewkey, this, _1), tr("Display private view key"));
  m_cmd_binder.set_handler("spendkey", boost::bind(&simple_wallet::spendkey, this, _1), tr("Display private spend key"));
  m_cmd_binder.set_handler("seed", boost::bind(&simple_wallet::seed, this, _1), tr("Display Electrum-style mnemonic seed"));
  m_cmd_binder.set_handler("set", boost::bind(&simple_wallet::set_variable, this, _1), tr("Available options: seed language - set wallet seed language; always-confirm-transfers <1|0> - whether to confirm unsplit txes; store-tx-info <1|0> - whether to store outgoing tx info (destination address, payment ID, tx secret key) for future reference; default-mixin <n> - set default mixin (default is 12); auto-refresh <1|0> - whether to automatically sync new blocks from the daemon; refresh-type <full|optimize-coinbase|no-coinbase|default> - set wallet refresh behaviour; priority [1|2|3|4|5] - normal/high(x2)/higher(x4)/elevated(x20)/forceful(x166) fee; confirm-missing-payment-id <1|0>"));
  m_cmd_binder.set_handler("rescan_spent", boost::bind(&simple_wallet::rescan_spent, this, _1), tr("Rescan blockchain for spent outputs"));
  m_cmd_binder.set_handler("get_tx_key", boost::bind(&simple_wallet::get_tx_key, this, _1), tr("Get transaction key (r) for a given <txid>"));
  m_cmd_binder.set_handler("check_tx_key", boost::bind(&simple_wallet::check_tx_key, this, _1), tr("Check amount going to <address> in <txid>"));
  m_cmd_binder.set_handler("show_transfers", boost::bind(&simple_wallet::show_transfers, this, _1), tr("show_transfers [in|out|all|pending|failed|pool] [index=<N1>[,<N2>,...]] [<min_height> [<max_height>]] - Show incoming/outgoing transfers within an optional height range"));
  //m_cmd_binder.set_handler("unspent_outputs", boost::bind(&simple_wallet::unspent_outputs, this, _1), tr("unspent_outputs [index=<N1>[,<N2>,...]] [<min_amount> [<max_amount>]] - Show unspent outputs of a specified address within an optional amount range"));
  m_cmd_binder.set_handler("rescan_bc", boost::bind(&simple_wallet::rescan_blockchain, this, _1), tr("Rescan blockchain from scratch"));
  m_cmd_binder.set_handler("set_tx_note", boost::bind(&simple_wallet::set_tx_note, this, _1), tr("Set an arbitrary string note for a txid"));
  m_cmd_binder.set_handler("get_tx_note", boost::bind(&simple_wallet::get_tx_note, this, _1), tr("Get a string note for a txid"));
  m_cmd_binder.set_handler("status", boost::bind(&simple_wallet::status, this, _1), tr("Show wallet status information"));
  m_cmd_binder.set_handler("sign", boost::bind(&simple_wallet::sign, this, _1), tr("Sign the contents of a file"));
  m_cmd_binder.set_handler("verify", boost::bind(&simple_wallet::verify, this, _1), tr("Verify a signature on the contents of a file"));
  m_cmd_binder.set_handler("export_key_images", boost::bind(&simple_wallet::export_key_images, this, _1), tr("Export a signed set of key images"));
  m_cmd_binder.set_handler("import_key_images", boost::bind(&simple_wallet::import_key_images, this, _1), tr("Import signed key images list and verify their spent status"));
  m_cmd_binder.set_handler("export_outputs", boost::bind(&simple_wallet::export_outputs, this, _1), tr("Export a set of outputs owned by this wallet"));
  m_cmd_binder.set_handler("import_outputs", boost::bind(&simple_wallet::import_outputs, this, _1), tr("Import set of outputs owned by this wallet"));
  m_cmd_binder.set_handler("help", boost::bind(&simple_wallet::help, this, _1), tr("Show this help"));
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_variable(const std::vector<std::string> &args)
{
  if (args.empty())
  {
    success_msg_writer() << "seed = " << m_wallet->get_seed_language();
    success_msg_writer() << "always-confirm-transfers = " << m_wallet->always_confirm_transfers();
    success_msg_writer() << "store-tx-info = " << m_wallet->store_tx_info();
    success_msg_writer() << "default-mixin = " << m_wallet->default_mixin();
    success_msg_writer() << "auto-refresh = " << m_wallet->auto_refresh();
    success_msg_writer() << "refresh-type = " << get_refresh_type_name(m_wallet->get_refresh_type());
    success_msg_writer() << "priority = " << m_wallet->get_default_priority();
    success_msg_writer() << "confirm-missing-payment-id = " << m_wallet->confirm_missing_payment_id();
    return true;
  }
  else
  {
    if (args[0] == "seed")
    {
      if (args.size() == 1)
      {
        fail_msg_writer() << tr("set seed: needs an argument. available options: language");
        return true;
      }
      else if (args[1] == "language")
      {
        std::vector<std::string> local_args = args;
        local_args.erase(local_args.begin(), local_args.begin()+2);
        seed_set_language(local_args);
        return true;
      }
    }
    else if (args[0] == "always-confirm-transfers")
    {
      if (args.size() <= 1)
      {
        fail_msg_writer() << tr("set always-confirm-transfers: needs an argument (0 or 1)");
        return true;
      }
      else
      {
        std::vector<std::string> local_args = args;
        local_args.erase(local_args.begin(), local_args.begin()+2);
        set_always_confirm_transfers(local_args);
        return true;
      }
    }
    else if (args[0] == "store-tx-info")
    {
      if (args.size() <= 1)
      {
        fail_msg_writer() << tr("set store-tx-info: needs an argument (0 or 1)");
        return true;
      }
      else
      {
        std::vector<std::string> local_args = args;
        local_args.erase(local_args.begin(), local_args.begin()+2);
        set_store_tx_info(local_args);
        return true;
      }
    }
    else if (args[0] == "default-mixin")
    {
      if (args.size() <= 1)
      {
        fail_msg_writer() << boost::format(tr("set default-mixin: needs an argument (integer >= %s and <= %)")) % DEFAULT_MIXIN % MAX_MIXIN;
        return true;
      }
      else
      {
        std::vector<std::string> local_args = args;
        local_args.erase(local_args.begin(), local_args.begin()+2);
        set_default_mixin(local_args);
        return true;
      }
    }
    else if (args[0] == "auto-refresh")
    {
      if (args.size() <= 1)
      {
        fail_msg_writer() << tr("set auto-refresh: needs an argument (0 or 1)");
        return true;
      }
      else
      {
        std::vector<std::string> local_args = args;
        local_args.erase(local_args.begin(), local_args.begin()+2);
        set_auto_refresh(local_args);
        return true;
      }
    }
    else if (args[0] == "refresh-type")
    {
      if (args.size() <= 1)
      {
        fail_msg_writer() << tr("set refresh-type: needs an argument:") <<
          tr("full (slowest, no assumptions); optimize-coinbase (fast, assumes the whole coinbase is paid to a single address); no-coinbase (fastest, assumes we receive no coinbase transaction), default (same as optimize-coinbase)");
        return true;
      }
      else
      {
        std::vector<std::string> local_args = args;
        local_args.erase(local_args.begin(), local_args.begin()+2);
        set_refresh_type(local_args);
        return true;
      }
    }
    else if (args[0] == "priority")
    {
      if (args.size() <= 1)
      {
        fail_msg_writer() << tr("set priority: needs an argument: 0, 1, 2, 3, 4 or 5");
        return true;
      }
      else
      {
        std::vector<std::string> local_args = args;
        local_args.erase(local_args.begin(), local_args.begin()+2);
        set_default_priority(local_args);
        return true;
      }
    }
    else if (args[0] == "confirm-missing-payment-id")
    {
      if (args.size() <= 1)
      {
        fail_msg_writer() << tr("set confirm-missing-payment-id: needs an argument (0 or 1)");
        return true;
      }
      else
      {
        std::vector<std::string> local_args = args;
        local_args.erase(local_args.begin(), local_args.begin()+2);
        set_confirm_missing_payment_id(local_args);
        return true;
      }
    }
  }
  fail_msg_writer() << tr("set: unrecognized argument(s)");
  return true;
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_log(const std::vector<std::string> &args)
{
  if(args.size() != 1)
  {
    fail_msg_writer() << tr("usage: set_log <log_level_number_0-4>");
    return true;
  }
  uint16_t l = 0;
  if(!epee::string_tools::get_xtype_from_string(l, args[0]))
  {
    fail_msg_writer() << tr("wrong number format, use: set_log <log_level_number_0-4>");
    return true;
  }
  if(LOG_LEVEL_4 < l)
  {
    fail_msg_writer() << tr("wrong number range, use: set_log <log_level_number_0-4>");
    return true;
  }

  log_space::log_singletone::get_set_log_detalisation_level(true, l);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::ask_wallet_create_if_needed()
{
  LOG_PRINT_L3("simple_wallet::ask_wallet_create_if_needed() started");
  std::string wallet_path;
  std::string confirm_creation;
  bool wallet_name_valid = false;
  bool keys_file_exists;
  bool wallet_file_exists;

  do{
      LOG_PRINT_L3("User asked to specify wallet file name.");
      wallet_path = command_line::input_line(
        tr(m_restoring ? "Specify a new wallet file name for your restored wallet (e.g., MyWallet).\n"
        "Wallet file name (or Ctrl-C to quit): " :
        "Specify wallet file name (e.g., MyWallet). If the wallet doesn't exist, it will be created.\n"
        "Wallet file name (or Ctrl-C to quit): ")
      );
      if(std::cin.eof())
      {
        LOG_ERROR("Unexpected std::cin.eof() - Exited simple_wallet::ask_wallet_create_if_needed()");
        return false;
      }
      if(!tools::wallet2::wallet_valid_path_format(wallet_path))
      {
        fail_msg_writer() << tr("Wallet name not valid. Please try again or use Ctrl-C to quit.");
        wallet_name_valid = false;
      }
      else
      {
        tools::wallet2::wallet_exists(wallet_path, keys_file_exists, wallet_file_exists);
        LOG_PRINT_L3("wallet_path: " << wallet_path << "");
        LOG_PRINT_L3("keys_file_exists: " << std::boolalpha << keys_file_exists << std::noboolalpha
        << "  wallet_file_exists: " << std::boolalpha << wallet_file_exists << std::noboolalpha);

        if((keys_file_exists || wallet_file_exists) && (!m_generate_new.empty() || m_restoring))
        {
          fail_msg_writer() << tr("Attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.");
          return false;
        }
        if(wallet_file_exists && keys_file_exists) //Yes wallet, yes keys
        {
          success_msg_writer() << tr("Wallet and key files found, loading...");
          m_wallet_file = wallet_path;
          return true;
        }
        else if(!wallet_file_exists && keys_file_exists) //No wallet, yes keys
        {
          success_msg_writer() << tr("Key file found but not wallet file. Regenerating...");
          m_wallet_file = wallet_path;
          return true;
        }
        else if(wallet_file_exists && !keys_file_exists) //Yes wallet, no keys
        {
          fail_msg_writer() << tr("Key file not found. Failed to open wallet: ") << "\"" << wallet_path << "\". Exiting.";
          return false;
        }
        else if(!wallet_file_exists && !keys_file_exists) //No wallet, no keys
        {
          message_writer() << tr(m_restoring ? "Confirm wallet name: " : "No wallet found with that name. Confirm creation of new wallet named: ") << wallet_path;
          confirm_creation = command_line::input_line(tr("(Y/Yes/N/No): "));
          if(std::cin.eof())
          {
            LOG_ERROR("Unexpected std::cin.eof() - Exited simple_wallet::ask_wallet_create_if_needed()");
            return false;
          }
          if(command_line::is_yes(confirm_creation))
          {
            success_msg_writer() << tr("Generating new wallet...");
            m_generate_new = wallet_path;
            return true;
          }
        }
      }
    } while(!wallet_name_valid);

  LOG_ERROR("Failed out of do-while loop in ask_wallet_create_if_needed()");
  return false;
}

/*!
 * \brief Prints the seed with a nice message
 * \param seed seed to print
 */
void simple_wallet::print_seed(std::string seed)
{
  success_msg_writer(true) << "\n" << tr("PLEASE NOTE: the following 26 words can be used to recover access to your wallet. "
    "Please write them down and store them somewhere safe and secure. Please do not store them in "
    "your email or on file storage services outside of your immediate control.\n");
  boost::replace_nth(seed, " ", 16, "\n");
  boost::replace_nth(seed, " ", 7, "\n");
  // don't log
  std::cout << seed << std::endl;
}
//----------------------------------------------------------------------------------------------------
static bool is_local_daemon(const std::string &address)
{
  // extract host
  epee::net_utils::http::url_content u_c;
  if (!epee::net_utils::parse_url(address, u_c))
  {
    LOG_PRINT_L1("Failed to determine whether daemon is local, assuming not");
    return false;
  }
  if (u_c.host.empty())
  {
    LOG_PRINT_L1("Failed to determine whether daemon is local, assuming not");
    return false;
  }

  // resolve to IP
  boost::asio::io_service io_service;
  boost::asio::ip::tcp::resolver resolver(io_service);
  boost::asio::ip::tcp::resolver::query query(u_c.host, "");
  boost::asio::ip::tcp::resolver::iterator i = resolver.resolve(query);
  while (i != boost::asio::ip::tcp::resolver::iterator())
  {
    const boost::asio::ip::tcp::endpoint &ep = *i;
    if (ep.address().is_loopback())
      return true;
    ++i;
  }

  return false;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::init(const boost::program_options::variables_map& vm)
{
  if (!handle_command_line(vm))
    return false;

  if((!m_generate_new.empty()) + (!m_wallet_file.empty()) + (!m_generate_from_view_key.empty()) + (!m_generate_from_keys.empty()) + (!m_generate_from_json.empty()) > 1)
  {
    fail_msg_writer() << tr("can't specify more than one of --generate-new-wallet=\"wallet_name\", --wallet-file=\"wallet_name\", --generate-from-view-key=\"wallet_name\", --generate-from-json=\"jsonfilename\" and --generate-from-keys=\"wallet_name\"");
    return false;
  }
  else if (m_generate_new.empty() && m_wallet_file.empty() && m_generate_from_view_key.empty() && m_generate_from_keys.empty() && m_generate_from_json.empty())
  {
    if(!ask_wallet_create_if_needed()) return false;
  }

  if (!m_generate_new.empty() || m_restoring)
  {
    std::string old_language;
    // check for recover flag.  if present, require electrum word list (only recovery option for now).
    if (m_restore_deterministic_wallet)
    {
      if (m_non_deterministic)
      {
        fail_msg_writer() << tr("can't specify both --restore-deterministic-wallet and --non-deterministic");
        return false;
      }

      if (m_electrum_seed.empty())
      {
        m_electrum_seed = command_line::input_line("Specify Electrum seed: ");
        if (std::cin.eof())
          return false;
        if (m_electrum_seed.empty())
        {
          fail_msg_writer() << tr("specify a recovery parameter with the --electrum-seed=\"words list here\"");
          return false;
        }
      }

      if (!crypto::ElectrumWords::words_to_bytes(m_electrum_seed, m_recovery_key, old_language))
      {
        fail_msg_writer() << tr("Electrum-style word list failed verification");
        return false;
      }
    }
    if (!m_restore_height && m_restoring)
    {
      std::string heightstr = command_line::input_line("Restore from specific blockchain height (optional, default 0): ");
      if (std::cin.eof())
        return false;
      if (heightstr.size())
      {
        try {
          m_restore_height = boost::lexical_cast<uint64_t>(heightstr);
        }
        catch (boost::bad_lexical_cast &) {
          fail_msg_writer() << tr("bad m_restore_height parameter:") << " " << heightstr;
          return false;
        }
      }
    }
    if (!m_generate_from_view_key.empty())
    {
      m_wallet_file = m_generate_from_view_key;
      // parse address
      std::string address_string = command_line::input_line("Standard address: ");
      if (std::cin.eof())
        return false;
      if (address_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      cryptonote::address_parse_info info;
      if (!get_account_address_from_str(info, tools::wallet2::has_testnet_option(vm), address_string))
      {
          fail_msg_writer() << tr("failed to parse address");
          return false;
      }
      if (info.is_subaddress)
      {
        fail_msg_writer() << tr("This address is a subaddress which cannot be used here.");
        return false;
      }

      // parse view secret key
      std::string viewkey_string = command_line::input_line("View key: ");
      if (std::cin.eof())
        return false;
      if (viewkey_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      cryptonote::blobdata viewkey_data;
      if(!epee::string_tools::parse_hexstr_to_binbuff(viewkey_string, viewkey_data))
      {
        fail_msg_writer() << tr("failed to parse view key secret key");
        return false;
      }
      crypto::secret_key viewkey = *reinterpret_cast<const crypto::secret_key*>(viewkey_data.data());

      m_wallet_file=m_generate_from_view_key;

      // check the view key matches the given address
      crypto::public_key pkey;
      if (!crypto::secret_key_to_public_key(viewkey, pkey)) {
        fail_msg_writer() << tr("failed to verify view key secret key");
        return false;
      }
      if (info.address.m_view_public_key != pkey) {
        fail_msg_writer() << tr("view key does not match standard address");
        return false;
      }

      bool r = new_wallet(vm, info.address, boost::none, viewkey);
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
    }
    else if (!m_generate_from_keys.empty())
    {
      m_wallet_file = m_generate_from_keys;
      // parse address
      std::string address_string = command_line::input_line("Standard address: ");
      if (std::cin.eof())
        return false;
      if (address_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      cryptonote::address_parse_info info;
      if (!get_account_address_from_str(info, tools::wallet2::has_testnet_option(vm), address_string))
      {
          fail_msg_writer() << tr("failed to parse address");
          return false;
      }
      if (info.is_subaddress)
      {
        fail_msg_writer() << tr("This address is a subaddress which cannot be used here.");
        return false;
      }

      // parse spend secret key
      std::string spendkey_string = command_line::input_line("Spend key: ");
      if (std::cin.eof())
        return false;
      if (spendkey_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      cryptonote::blobdata spendkey_data;
      if(!epee::string_tools::parse_hexstr_to_binbuff(spendkey_string, spendkey_data))
      {
        fail_msg_writer() << tr("failed to parse spend key secret key");
        return false;
      }
      crypto::secret_key spendkey = *reinterpret_cast<const crypto::secret_key*>(spendkey_data.data());

      // parse view secret key
      std::string viewkey_string = command_line::input_line("View key: ");
      if (std::cin.eof())
        return false;
      if (viewkey_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      cryptonote::blobdata viewkey_data;
      if(!epee::string_tools::parse_hexstr_to_binbuff(viewkey_string, viewkey_data))
      {
        fail_msg_writer() << tr("failed to parse view key secret key");
        return false;
      }
      crypto::secret_key viewkey = *reinterpret_cast<const crypto::secret_key*>(viewkey_data.data());

      m_wallet_file=m_generate_from_keys;

      // check the spend and view keys match the given address
      crypto::public_key pkey;
      if (!crypto::secret_key_to_public_key(spendkey, pkey)) {
        fail_msg_writer() << tr("failed to verify spend key secret key");
        return false;
      }
      if (info.address.m_spend_public_key != pkey) {
        fail_msg_writer() << tr("spend key does not match standard address");
        return false;
      }
      if (!crypto::secret_key_to_public_key(viewkey, pkey)) {
        fail_msg_writer() << tr("failed to verify view key secret key");
        return false;
      }
      if (info.address.m_view_public_key != pkey) {
        fail_msg_writer() << tr("view key does not match standard address");
        return false;
      }
      bool r = new_wallet(vm, info.address, spendkey, viewkey);
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
    }
    else if (!m_generate_from_json.empty())
    {
      m_wallet_file = m_generate_from_json;
      m_wallet = tools::wallet2::make_from_json(vm, m_wallet_file);
      if (!m_wallet)
        return false;
    }
    else
    {
      m_wallet_file = m_generate_new;
      bool r = new_wallet(vm, m_recovery_key, m_restore_deterministic_wallet, m_non_deterministic, old_language);
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
    }
  }
  else
  {
    assert(!m_wallet_file.empty());
    bool r = open_wallet(vm);
    CHECK_AND_ASSERT_MES(r, false, tr("failed to open account"));
  }
  if (!m_wallet)
  {
    fail_msg_writer() << tr("wallet is null");
    return false;
  }

  // set --trusted-daemon if local
  try
  {
    if (is_local_daemon(m_wallet->get_daemon_address()))
    {
      LOG_PRINT_L1(tr("Daemon is local, assuming trusted"));
      m_trusted_daemon = true;
    }
  }
  catch (const std::exception &e) { }

  m_wallet->callback(this);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::deinit()
{
  if (!m_wallet.get())
    return true;

  return close_wallet();
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::handle_command_line(const boost::program_options::variables_map& vm)
{
  m_wallet_file                   = command_line::get_arg(vm, arg_wallet_file);
  m_generate_new                  = command_line::get_arg(vm, arg_generate_new_wallet);
  m_generate_from_view_key        = command_line::get_arg(vm, arg_generate_from_view_key);
  m_generate_from_keys            = command_line::get_arg(vm, arg_generate_from_keys);
  m_generate_from_json            = command_line::get_arg(vm, arg_generate_from_json);
  m_electrum_seed                 = command_line::get_arg(vm, arg_electrum_seed);
  m_restore_deterministic_wallet  = command_line::get_arg(vm, arg_restore_deterministic_wallet);
  m_non_deterministic             = command_line::get_arg(vm, arg_non_deterministic);
  m_trusted_daemon                = command_line::get_arg(vm, arg_trusted_daemon);
  m_allow_mismatched_daemon_version = command_line::get_arg(vm, arg_allow_mismatched_daemon_version);
  m_restore_height                = command_line::get_arg(vm, arg_restore_height);
  m_restoring                     = !m_generate_from_view_key.empty() ||
                                    !m_generate_from_keys.empty() ||
                                    !m_generate_from_json.empty() ||
                                    m_restore_deterministic_wallet;

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::try_connect_to_daemon(bool silent)
{
  uint32_t version = 0;
  if (!m_wallet->check_connection(&version))
  {
    if (!silent)
      fail_msg_writer() << tr("wallet failed to connect to daemon: ") << m_wallet->get_daemon_address() << ". " <<
        tr("Daemon either is not started or wrong port was passed. "
        "Please make sure daemon is running or restart the wallet with the correct daemon address.");
    return false;
  }
  if (!m_allow_mismatched_daemon_version && ((version >> 16) != CORE_RPC_VERSION_MAJOR))
  {
    if (!silent)
      fail_msg_writer() << boost::format(tr("Daemon uses a different RPC major version (%u) than the wallet (%u): %s. Either update one of them, or use --allow-mismatched-daemon-version.")) % (version>>16) % CORE_RPC_VERSION_MAJOR % m_wallet->get_daemon_address();
    return false;
  }
  return true;
}

/*!
 * \brief Gets the word seed language from the user.
 *
 * User is asked to choose from a list of supported languages.
 *
 * \return The chosen language.
 */
std::string simple_wallet::get_mnemonic_language()
{
  std::vector<std::string> language_list;
  std::string language_choice;
  int language_number = -1;
  crypto::ElectrumWords::get_language_list(language_list);
  std::cout << tr("List of available languages for your wallet's seed:") << std::endl;
  int ii;
  std::vector<std::string>::iterator it;
  for (it = language_list.begin(), ii = 0; it != language_list.end(); it++, ii++)
  {
    std::cout << ii << " : " << *it << std::endl;
  }
  while (language_number < 0)
  {
    language_choice = command_line::input_line(tr("Enter the number corresponding to the language of your choice: "));
    if (std::cin.eof())
      return std::string();
    try
    {
      language_number = std::stoi(language_choice);
      if (!((language_number >= 0) && (static_cast<unsigned int>(language_number) < language_list.size())))
      {
        language_number = -1;
        fail_msg_writer() << tr("invalid language choice passed. Please try again.\n");
      }
    }
    catch (std::exception &e)
    {
      fail_msg_writer() << tr("invalid language choice passed. Please try again.\n");
    }
  }
  return language_list[language_number];
}
//----------------------------------------------------------------------------------------------------
boost::optional<tools::password_container> simple_wallet::get_and_verify_password() const
{
  auto pwd_container = tools::wallet2::password_prompt(m_wallet_file.empty());
  if (!pwd_container)
    return boost::none;

  if (!m_wallet->verify_password(pwd_container->password()))
  {
    fail_msg_writer() << tr("invalid password");
    return boost::none;
  }
  return pwd_container;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::new_wallet(const boost::program_options::variables_map& vm,
  const crypto::secret_key& recovery_key, bool recover, bool two_random, const std::string &old_language)
{
  auto rc = tools::wallet2::make_new(vm);
  m_wallet = std::move(rc.first);
  if (!m_wallet)
  {
    return false;
  }


  std::string mnemonic_language = old_language;
  // Ask for seed language if:
  // it's a deterministic wallet AND
  // that was earlier used before this restore)
  if ((!two_random) && (!m_restore_deterministic_wallet))
  {

    mnemonic_language = get_mnemonic_language();
    if (mnemonic_language.empty())
      return false;
  }

  m_wallet->set_seed_language(mnemonic_language);

  // for a totally new account, we don't care about older blocks.
  if (!m_restoring)
  {
    std::string err;
    m_wallet->set_refresh_from_block_height(get_daemon_blockchain_height(err));
  } else if (m_restore_height)
  {
    m_wallet->set_refresh_from_block_height(m_restore_height);
  }

  crypto::secret_key recovery_val;
  try
  {
    recovery_val = m_wallet->generate(m_wallet_file, std::move(rc.second).password(), recovery_key, recover, two_random);
    message_writer(epee::log_space::console_color_white, true) << tr("Generated new wallet: ")
      << m_wallet->get_account().get_public_address_str(m_wallet->testnet());
    std::cout << tr("View key: ") << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_view_secret_key) << ENDL;
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << tr("failed to generate new wallet: ") << e.what();
    return false;
  }

  // convert rng value to electrum-style word list
  std::string electrum_words;

  crypto::ElectrumWords::bytes_to_words(recovery_val, electrum_words, mnemonic_language);

  success_msg_writer() <<
    "**********************************************************************\n" <<
    tr("Your wallet has been generated!\n"
    "To start synchronizing with the daemon, use \"refresh\" command.\n"
    "Use \"help\" command to see the list of available commands.\n"
    "Always use \"exit\" command when closing sumo-wallet-cli to save your\n"
    "current session's state. Otherwise, you might need to synchronize \n"
    "your wallet again (your wallet keys are NOT at risk in any case).\n")
  ;

  if (!two_random)
  {
    print_seed(electrum_words);
  }
  success_msg_writer() << "**********************************************************************";

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::new_wallet(const boost::program_options::variables_map& vm,
  const cryptonote::account_public_address& address, const boost::optional<crypto::secret_key>& spendkey,
  const crypto::secret_key& viewkey)
{
  auto rc = tools::wallet2::make_new(vm);
  m_wallet = std::move(rc.first);
  if (!m_wallet)
  {
    return false;
  }
  if (m_restore_height)
    m_wallet->set_refresh_from_block_height(m_restore_height);

  try
  {
    if (spendkey)
    {
      m_wallet->generate(m_wallet_file, std::move(rc.second).password(), address, *spendkey, viewkey);
    }
    else
    {
      m_wallet->generate(m_wallet_file, std::move(rc.second).password(), address, viewkey);
    }
    message_writer(epee::log_space::console_color_white, true) << tr("Generated new wallet: ")
      << m_wallet->get_account().get_public_address_str(m_wallet->testnet());
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << tr("failed to generate new wallet: ") << e.what();
    return false;
  }


  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::open_wallet(const boost::program_options::variables_map& vm)
{
  if (!tools::wallet2::wallet_valid_path_format(m_wallet_file))
  {
    fail_msg_writer() << tr("wallet file path not valid: ") << m_wallet_file;
    return false;
  }
  std::string password;
  try
  {
    auto rc = tools::wallet2::make_from_file(vm, m_wallet_file);
    m_wallet = std::move(rc.first);
    password = std::move(rc.second).password();
    if (!m_wallet)
    {
      return false;
    }

    message_writer(epee::log_space::console_color_white, true) <<
      (m_wallet->watch_only() ? tr("Opened watch-only wallet") : tr("Opened wallet")) << ": "
      << m_wallet->get_account().get_public_address_str(m_wallet->testnet());
    // If the wallet file is deprecated, we should ask for mnemonic language again and store
    // everything in the new format.
    // NOTE: this is_deprecated() refers to the wallet file format before becoming JSON. It does not refer to the "old english" seed words form of "deprecated" used elsewhere.
    if (m_wallet->is_deprecated())
    {
      if (m_wallet->is_deterministic())
      {
        message_writer(epee::log_space::console_color_green, false) << "\n" << tr("You had been using "
          "a deprecated version of the wallet. Please proceed to upgrade your wallet.\n");
        std::string mnemonic_language = get_mnemonic_language();
        if (mnemonic_language.empty())
          return false;
        m_wallet->set_seed_language(mnemonic_language);
        m_wallet->rewrite(m_wallet_file, password);

        // Display the seed
        std::string seed;
        m_wallet->get_seed(seed);
        print_seed(seed);
      }
      else
      {
        message_writer(epee::log_space::console_color_green, false) << "\n" << tr("You had been using "
          "a deprecated version of the wallet. Your wallet file format is being upgraded now.\n");
        m_wallet->rewrite(m_wallet_file, password);
      }
    }
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << tr("failed to load wallet: ") << e.what();
    if (m_wallet)
    {
      // only suggest removing cache if the password was actually correct
      bool password_is_correct = false;
      try
      {
        password_is_correct = m_wallet->verify_password(password);
      }
      catch (...) {} // guard against I/O errors
      if (password_is_correct)
        fail_msg_writer() << boost::format(tr("You may want to remove the file \"%s\" and try again")) % m_wallet_file;
    }
    return false;
  }
  success_msg_writer() <<
    "**********************************************************************\n" <<
    tr("Use \"help\" command to see the list of available commands.\n") <<
    "**********************************************************************";
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::close_wallet()
{
  if (m_idle_run.load(std::memory_order_relaxed))
  {
    m_idle_run.store(false, std::memory_order_relaxed);
    m_wallet->stop();
    {
      boost::unique_lock<boost::mutex> lock(m_idle_mutex);
      m_idle_cond.notify_one();
    }
    m_idle_thread.join();
  }

  bool r = m_wallet->deinit();
  if (!r)
  {
    fail_msg_writer() << tr("failed to deinitialize wallet");
    return false;
  }

  try
  {
    m_wallet->store();
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << e.what();
    return false;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::save(const std::vector<std::string> &args)
{
  try
  {
    LOCK_IDLE_SCOPE();
    m_wallet->store();
    success_msg_writer() << tr("Wallet data saved");
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << e.what();
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::save_watch_only(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = tools::password_container::prompt(true, tr("Password for new watch-only wallet"));

  if (!pwd_container)
  {
    fail_msg_writer() << tr("failed to read wallet password");
    return true;
  }

  m_wallet->write_watch_only_wallet(m_wallet_file, pwd_container->password());
  return true;
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::start_mining(const std::vector<std::string>& args)
{
  if (!m_trusted_daemon)
  {
    fail_msg_writer() << tr("this command requires a trusted daemon. Enable with --trusted-daemon");
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  assert(m_wallet);
  COMMAND_RPC_START_MINING::request req;
  req.miner_address = m_wallet->get_account().get_public_address_str(m_wallet->testnet());

  bool ok = true;
  size_t max_mining_threads_count = (std::max)(tools::get_max_concurrency(), static_cast<unsigned>(2));
  if (0 == args.size())
  {
    req.threads_count = 1;
  }
  else if (1 == args.size())
  {
    uint16_t num = 1;
    ok = string_tools::get_xtype_from_string(num, args[0]);
    ok = ok && (1 <= num && num <= max_mining_threads_count);
    req.threads_count = num;
  }
  else
  {
    ok = false;
  }

  if (!ok)
  {
    fail_msg_writer() << tr("invalid arguments. Please use start_mining [<number_of_threads>], "
      "<number_of_threads> should be from 1 to ") << max_mining_threads_count;
    return true;
  }

  COMMAND_RPC_START_MINING::response res;
  bool r = net_utils::invoke_http_json_remote_command2(m_wallet->get_daemon_address() + "/start_mining", req, res, m_http_client);
  std::string err = interpret_rpc_response(r, res.status);
  if (err.empty())
    success_msg_writer() << tr("Mining started in daemon");
  else
    fail_msg_writer() << tr("mining has NOT been started: ") << err;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::stop_mining(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  assert(m_wallet);
  COMMAND_RPC_STOP_MINING::request req;
  COMMAND_RPC_STOP_MINING::response res;
  bool r = net_utils::invoke_http_json_remote_command2(m_wallet->get_daemon_address() + "/stop_mining", req, res, m_http_client);
  std::string err = interpret_rpc_response(r, res.status);
  if (err.empty())
    success_msg_writer() << tr("Mining stopped in daemon");
  else
    fail_msg_writer() << tr("mining has NOT been stopped: ") << err;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::save_bc(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  assert(m_wallet);
  COMMAND_RPC_SAVE_BC::request req;
  COMMAND_RPC_SAVE_BC::response res;
  bool r = net_utils::invoke_http_json_remote_command2(m_wallet->get_daemon_address() + "/save_bc", req, res, m_http_client);
  std::string err = interpret_rpc_response(r, res.status);
  if (err.empty())
    success_msg_writer() << tr("Blockchain saved");
  else
    fail_msg_writer() << tr("blockchain can't be saved: ") << err;
  return true;
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_new_block(uint64_t height, const cryptonote::block& block)
{
  if (!m_auto_refresh_refreshing)
    m_refresh_progress_reporter.update(height, false);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_money_received(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t amount, const cryptonote::subaddress_index& subaddr_index)
{
  message_writer(epee::log_space::console_color_green, false) << "\r" <<
    tr("Height ") << height << ", " <<
    tr("txid ") << txid << ", " <<
    tr("received ") << print_money(amount) << ", " <<
    tr("idx ") << subaddr_index;
  if (m_auto_refresh_refreshing)
    m_cmd_binder.print_prompt();
  else
    m_refresh_progress_reporter.update(height, true);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_money_spent(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& in_tx, uint64_t amount, const cryptonote::transaction& spend_tx, const cryptonote::subaddress_index& subaddr_index)
{
  message_writer(epee::log_space::console_color_magenta, false) << "\r" <<
    tr("Height ") << height << ", " <<
    tr("txid ") << txid << ", " <<
    tr("spent ") << print_money(amount) << ", " <<
    tr("idx ") << subaddr_index;
  if (m_auto_refresh_refreshing)
    m_cmd_binder.print_prompt();
  else
    m_refresh_progress_reporter.update(height, true);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_skip_transaction(uint64_t height, const cryptonote::transaction& tx)
{
  message_writer(epee::log_space::console_color_red, true) << "\r" <<
    tr("Height ") << height << ", " <<
    tr("transaction ") << get_transaction_hash(tx) << ", " <<
    tr("unsupported transaction format");
  if (m_auto_refresh_refreshing)
    m_cmd_binder.print_prompt();
  else
    m_refresh_progress_reporter.update(height, true);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::refresh_main(uint64_t start_height, bool reset, bool is_init)
{
  if (!try_connect_to_daemon())
    return true;

  LOCK_IDLE_SCOPE();

  if (reset)
    m_wallet->rescan_blockchain(false);

  message_writer() << tr("Starting refresh...");

  uint64_t fetched_blocks = 0;
  bool ok = false;
  std::ostringstream ss;
  try
  {
    m_in_manual_refresh.store(true, std::memory_order_relaxed);
    epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){m_in_manual_refresh.store(false, std::memory_order_relaxed);});
    m_wallet->refresh(start_height, fetched_blocks);
    ok = true;
    // Clear line "Height xxx of xxx"
    std::cout << "\r                                                                \r";
    success_msg_writer(true) << tr("Refresh done, blocks received: ") << fetched_blocks;
    if (is_init)
      print_accounts();
    show_balance_unlocked();
  }
  catch (const tools::error::daemon_busy&)
  {
    ss << tr("daemon is busy. Please try again later.");
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    ss << tr("no connection to daemon. Please make sure daemon is running.");
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("RPC error: " << e.to_string());
    ss << tr("RPC error: ") << e.what();
  }
  catch (const tools::error::refresh_error& e)
  {
    LOG_ERROR("refresh error: " << e.to_string());
    ss << tr("refresh error: ") << e.what();
  }
  catch (const tools::error::wallet_internal_error& e)
  {
    LOG_ERROR("internal error: " << e.to_string());
    ss << tr("internal error: ") << e.what();
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    ss << tr("unexpected error: ") << e.what();
  }
  catch (...)
  {
    LOG_ERROR("unknown error");
    ss << tr("unknown error");
  }

  if (!ok)
  {
    fail_msg_writer() << tr("refresh failed: ") << ss.str() << ". " << tr("Blocks received: ") << fetched_blocks;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::refresh(const std::vector<std::string>& args)
{
  uint64_t start_height = 0;
  if(!args.empty()){
    try
    {
        start_height = boost::lexical_cast<uint64_t>( args[0] );
    }
    catch(const boost::bad_lexical_cast &)
    {
        start_height = 0;
    }
  }
  return refresh_main(start_height, false);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_balance_unlocked(bool detailed)
{
  success_msg_writer() << tr("Currently selected account: [") << m_current_subaddress_account << tr("] ") << m_wallet->get_subaddress_label({ m_current_subaddress_account, 0 });
  success_msg_writer() << tr("Balance: ") << print_money(m_wallet->balance(m_current_subaddress_account)) << ", "
    << tr("unlocked balance: ") << print_money(m_wallet->unlocked_balance(m_current_subaddress_account));
  std::map<uint32_t, uint64_t> balance_per_subaddress = m_wallet->balance_per_subaddress(m_current_subaddress_account);
  std::map<uint32_t, uint64_t> unlocked_balance_per_subaddress = m_wallet->unlocked_balance_per_subaddress(m_current_subaddress_account);
  if (!detailed || balance_per_subaddress.empty())
    return true;
  success_msg_writer() << tr("Balance per address:");
  success_msg_writer() << boost::format("%18s %21s %21s %8s %21s") % tr("Address") % tr("Balance") % tr("Unlocked balance") % tr("Outputs") % tr("Label");
  std::vector<tools::wallet2::transfer_details> transfers;
  m_wallet->get_transfers(transfers);
  for (const auto& i : balance_per_subaddress)
  {
    cryptonote::subaddress_index subaddr_index = { m_current_subaddress_account, i.first };
    std::string address_str = m_wallet->get_subaddress_as_str(subaddr_index).substr(0, 9);
    uint64_t num_unspent_outputs = std::count_if(transfers.begin(), transfers.end(), [&subaddr_index](const tools::wallet2::transfer_details& td) { return !td.m_spent && td.m_subaddr_index == subaddr_index; });
    success_msg_writer() << boost::format(tr("%8u %9s %21s %21s %8u %21s")) % i.first % address_str % print_money(i.second) % print_money(unlocked_balance_per_subaddress[i.first]) % num_unspent_outputs % m_wallet->get_subaddress_label(subaddr_index).substr(0, 21);
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_balance(const std::vector<std::string>& args/* = std::vector<std::string>()*/)
{
  if (args.size() > 1 || (args.size() == 1 && args[0] != "detail"))
  {
    fail_msg_writer() << tr("usage: balance [detail]");
    return true;
  }
  LOCK_IDLE_SCOPE();
  show_balance_unlocked(args.size() == 1);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_incoming_transfers(const std::vector<std::string>& args)
{
  if (args.size() > 3)
  {
    fail_msg_writer() << tr("usage: incoming_transfers [available|unavailable] [verbose] [index=<N>]");
    return true;
  }
  auto local_args = args;
  LOCK_IDLE_SCOPE();

  bool filter = false;
  bool available = false;
  bool verbose = false;
  if (local_args.size() > 0)
  {
    if (local_args[0] == "available")
    {
      filter = true;
      available = true;
      local_args.erase(local_args.begin());
    }
    else if (local_args[0] == "unavailable")
    {
      filter = true;
      available = false;
      local_args.erase(local_args.begin());
    }
  }
  if (local_args.size() > 0 && local_args[0] == "verbose")
  {
    verbose = true;
    local_args.erase(local_args.begin());
  }

  std::set<uint32_t> subaddr_indices;
  if (local_args.size() > 0 && local_args[0].substr(0, 6) == "index=")
  {
    if (!parse_subaddress_indices(local_args[0], subaddr_indices))
      return true;
  }

  tools::wallet2::transfer_container transfers;
  m_wallet->get_transfers(transfers);

  bool transfers_found = false;
  for (const auto& td : transfers)
  {
    if (!filter || available != td.m_spent)
    {
      if (m_current_subaddress_account != td.m_subaddr_index.major || (!subaddr_indices.empty() && subaddr_indices.count(td.m_subaddr_index.minor) == 0))
        continue;
      if (!transfers_found)
      {
        std::string verbose_string;
        if (verbose)
          verbose_string = (boost::format("%68s%68s") % tr("pubkey") % tr("key image")).str();
        message_writer() << boost::format("%21s%8s%12s%8s%16s%68s%16s%s") % tr("amount") % tr("spent") % tr("unlocked") % tr("ringct") % tr("global index") % tr("tx id") % tr("addr index") % verbose_string;
        transfers_found = true;
      }
      std::string verbose_string;
      if (verbose)
        verbose_string = (boost::format("%68s%68s") % td.get_public_key() % (td.m_key_image_known ? epee::string_tools::pod_to_hex(td.m_key_image) : std::string('?', 64))).str();
      message_writer(td.m_spent ? epee::log_space::console_color_magenta : epee::log_space::console_color_green, false) <<
        boost::format("%21s%8s%12s%8s%16u%68s%16u%s") %
        print_money(td.amount()) %
        (td.m_spent ? tr("T") : tr("F")) %
        (m_wallet->is_transfer_unlocked(td) ? tr("unlocked") : tr("locked")) %
        (td.is_rct() ? tr("RingCT") : tr("-")) %
        td.m_global_output_index %
        td.m_txid %
        td.m_subaddr_index.minor %
        verbose_string;
    }
  }

  if (!transfers_found)
  {
    if (!filter)
    {
      success_msg_writer() << tr("No incoming transfers");
    }
    else if (available)
    {
      success_msg_writer() << tr("No incoming available transfers");
    }
    else
    {
      success_msg_writer() << tr("No incoming unavailable transfers");
    }
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_payments(const std::vector<std::string> &args)
{
  if(args.empty())
  {
    fail_msg_writer() << tr("expected at least one payment_id");
    return true;
  }

  LOCK_IDLE_SCOPE();

  message_writer() << boost::format("%68s%68s%12s%21s%16s%16s") %
    tr("payment") % tr("transaction") % tr("height") % tr("amount") % tr("unlock time") % tr("addr index");

  bool payments_found = false;
  for(std::string arg : args)
  {
    crypto::hash payment_id;
    if(tools::wallet2::parse_payment_id(arg, payment_id))
    {
      std::list<tools::wallet2::payment_details> payments;
      m_wallet->get_payments(payment_id, payments);
      if(payments.empty())
      {
        success_msg_writer() << tr("No payments with id ") << payment_id;
        continue;
      }

      for (const tools::wallet2::payment_details& pd : payments)
      {
        if(!payments_found)
        {
          payments_found = true;
        }
        success_msg_writer(true) <<
          boost::format("%68s%68s%12s%21s%16s%16s") %
          payment_id %
          pd.m_tx_hash %
          pd.m_block_height %
          print_money(pd.m_amount) %
          pd.m_unlock_time %
          pd.m_subaddr_index.minor;
      }
    }
    else
    {
      fail_msg_writer() << tr("payment ID has invalid format, expected 16 or 64 character hex string: ") << arg;
    }
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
uint64_t simple_wallet::get_daemon_blockchain_height(std::string& err)
{
  if (!m_wallet)
  {
    throw std::runtime_error("simple_wallet null wallet");
  }

  COMMAND_RPC_GET_HEIGHT::request req;
  COMMAND_RPC_GET_HEIGHT::response res = boost::value_initialized<COMMAND_RPC_GET_HEIGHT::response>();
  bool r = net_utils::invoke_http_json_remote_command2(m_wallet->get_daemon_address() + "/getheight", req, res, m_http_client);
  err = interpret_rpc_response(r, res.status);
  return res.height;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_blockchain_height(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  std::string err;
  uint64_t bc_height = get_daemon_blockchain_height(err);
  if (err.empty())
    success_msg_writer() << bc_height;
  else
    fail_msg_writer() << tr("failed to get blockchain height: ") << err;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::rescan_spent(const std::vector<std::string> &args)
{
  if (!m_trusted_daemon)
  {
    fail_msg_writer() << tr("this command requires a trusted daemon. Enable with --trusted-daemon");
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  try
  {
    LOCK_IDLE_SCOPE();
    m_wallet->rescan_spent();
  }
  catch (const tools::error::daemon_busy&)
  {
    fail_msg_writer() << tr("daemon is busy. Please try again later.");
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    fail_msg_writer() << tr("no connection to daemon. Please make sure daemon is running.");
  }
  catch (const tools::error::is_key_image_spent_error&)
  {
    fail_msg_writer() << tr("failed to get spent status");
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("RPC error: " << e.to_string());
    fail_msg_writer() << tr("RPC error: ") << e.what();
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    fail_msg_writer() << tr("unexpected error: ") << e.what();
  }
  catch (...)
  {
    LOG_ERROR("unknown error");
    fail_msg_writer() << tr("unknown error");
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::transfer_main(int transfer_type, const std::vector<std::string> &args_, bool retry, float tx_size_target_factor)
{
  //  "transfer [index=<N1>[,<N2>,...]] [<priority>] [<mixin_count>] <address> <amount> [<payment_id>]"
  if (!retry)
    if (!get_and_verify_password()) { return true; }


  if (!try_connect_to_daemon())
    return true;

  if (!retry){
    LOCK_IDLE_SCOPE();
  }

  std::vector<std::string> local_args = args_;

  std::set<uint32_t> subaddr_indices;
  if (local_args.size() > 0 && local_args[0].substr(0, 6) == "index=")
  {
    if (!parse_subaddress_indices(local_args[0], subaddr_indices))
      return true;
    local_args.erase(local_args.begin());
  }

  int priority = 0;
  if (local_args.size() > 0) {
    auto priority_pos = std::find(
      allowed_priority_strings.begin(),
      allowed_priority_strings.end(),
      local_args[0]);
    if (priority_pos != allowed_priority_strings.end()) {
      local_args.erase(local_args.begin());
      priority = std::distance(allowed_priority_strings.begin(), priority_pos);
    }
  }

  size_t fake_outs_count;
  if(local_args.size() > 0) {
		if (!epee::string_tools::get_xtype_from_string(fake_outs_count, local_args[0]))
    {
			message_writer() << boost::format(tr("** No mixin value specified, default mixin %s will be used for this transaction.")) % (m_wallet->default_mixin() > 0 ? m_wallet->default_mixin() : DEFAULT_MIXIN);
			fake_outs_count = m_wallet->default_mixin() > 0 ? m_wallet->default_mixin() : DEFAULT_MIXIN;
    }
    else
    {
      if (transfer_type != TransferOriginal && (fake_outs_count < DEFAULT_MIXIN || fake_outs_count > MAX_MIXIN))
			{
				std::stringstream prompt;
        if (fake_outs_count < DEFAULT_MIXIN){
          prompt << boost::format(tr("Given mixin value %s is too low, default mixin %s will be used for this transaction. Is this okay?  (Y/Yes/N/No): ")) % fake_outs_count % (m_wallet->default_mixin() > 0 ? m_wallet->default_mixin() : DEFAULT_MIXIN);
        }
        else{
          fail_msg_writer() << boost::format(tr("Given mixin value %s is too high. Max mixin value allowed is %s. Please resend tx with lower mixin.")) % fake_outs_count % MAX_MIXIN;
          return true;
        }

				std::string accepted = command_line::input_line(prompt.str());
				if (std::cin.eof())
					return true;

				if (!command_line::is_yes(accepted))
				{
					fail_msg_writer() << tr("transaction cancelled.");
					return true;
				}

				fake_outs_count = m_wallet->default_mixin() > 0 ? m_wallet->default_mixin() : DEFAULT_MIXIN;
			}

			local_args.erase(local_args.begin());
    }

	}

  const size_t min_args = (transfer_type == TransferLocked) ? 3 : 2;
  if(local_args.size() < min_args)
  {
     fail_msg_writer() << tr("wrong number of arguments");
     return true;
  }

  std::vector<uint8_t> extra;
  bool payment_id_seen = false;
  bool expect_even = (transfer_type == TransferLocked);
  if ((expect_even ? 0 : 1) == local_args.size() % 2)
  {
    std::string payment_id_str = local_args.back();
    local_args.pop_back();

    crypto::hash payment_id;
    bool r = tools::wallet2::parse_long_payment_id(payment_id_str, payment_id);
    if(r)
    {
      std::string extra_nonce;
      set_payment_id_to_tx_extra_nonce(extra_nonce, payment_id);
      r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
    }
    else
    {
      crypto::hash8 payment_id8;
      r = tools::wallet2::parse_short_payment_id(payment_id_str, payment_id8);
      if(r)
      {
        std::string extra_nonce;
        set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, payment_id8);
        r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
      }
    }

    if(!r)
    {
      fail_msg_writer() << tr("payment id has invalid format, expected 16 or 64 character hex string: ") << payment_id_str;
      return true;
    }
    payment_id_seen = true;
  }

  uint64_t locked_blocks = 0;
  if (transfer_type == TransferLocked)
  {
    try
    {
      locked_blocks = boost::lexical_cast<uint64_t>(local_args.back());
    }
    catch (const std::exception &e)
    {
      fail_msg_writer() << tr("bad locked_blocks parameter:") << " " << local_args.back();
      return true;
    }
    if (locked_blocks > 1000000)
    {
      fail_msg_writer() << tr("Locked blocks too high, max 1000000 (about 8 years)");
      return true;
    }
    local_args.pop_back();
  }

  //bool is_subaddress = false;
  vector<cryptonote::tx_destination_entry> dsts;
  for (size_t i = 0; i < local_args.size(); i += 2)
  {
    cryptonote::address_parse_info info;
    cryptonote::tx_destination_entry de;
    if (!cryptonote::get_account_address_from_str_or_url(info, m_wallet->testnet(), local_args[i]))
      return true;

    de.is_subaddress = info.is_subaddress;
    de.addr = info.address;

    if (info.has_payment_id)
    {
      if (payment_id_seen)
      {
        fail_msg_writer() << tr("a single transaction cannot use more than one payment id: ") << local_args[i];
        return true;
      }

      std::string extra_nonce;
      set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, info.payment_id);
      bool r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
      if(!r)
      {
        fail_msg_writer() << tr("failed to set up payment id, though it was decoded correctly");
        return true;
      }
      payment_id_seen = true;
    }

    bool ok = cryptonote::parse_amount(de.amount, local_args[i + 1]);
    if(!ok || 0 == de.amount)
    {
      fail_msg_writer() << tr("amount is wrong: ") << local_args[i] << ' ' << local_args[i + 1] <<
        ", " << tr("expected number from 0 to ") << print_money(std::numeric_limits<uint64_t>::max());
      return true;
    }

    dsts.push_back(de);
  }

  // prompt is there is no payment id and confirmation is required
  if (!retry && !payment_id_seen && m_wallet->confirm_missing_payment_id())
  {
     std::string accepted = command_line::input_line(tr("No payment id is included with this transaction. Is this okay?  (Y/Yes/N/No): "));
     if (std::cin.eof())
       return true;
     if (!command_line::is_yes(accepted))
     {
       fail_msg_writer() << tr("transaction cancelled.");

       // would like to return false, because no tx made, but everything else returns true
       // and I don't know what returning false might adversely affect.  *sigh*
       return true;
     }
  }

  retry = false;
  try
  {
    // figure out what tx will be necessary
    std::vector<tools::wallet2::pending_tx> ptx_vector;
    uint64_t bc_height, unlock_block = 0;
    std::string err;
    switch (transfer_type)
    {
      case TransferLocked:
        bc_height = get_daemon_blockchain_height(err);
        if (!err.empty())
        {
          fail_msg_writer() << tr("failed to get blockchain height: ") << err;
          return true;
        }
        unlock_block = bc_height + locked_blocks;
        ptx_vector = m_wallet->create_transactions_2(dsts, fake_outs_count, unlock_block /* unlock_time */, priority, extra, m_current_subaddress_account, subaddr_indices, m_trusted_daemon, tx_size_target_factor);
      break;
      case TransferNew:
        ptx_vector = m_wallet->create_transactions_2(dsts, fake_outs_count, 0 /* unlock_time */, priority, extra, m_current_subaddress_account, subaddr_indices, m_trusted_daemon, tx_size_target_factor);
      break;
      default:
        LOG_ERROR("Unknown transfer method, using original");
      case TransferOriginal:
        ptx_vector = m_wallet->create_transactions(dsts, fake_outs_count, 0 /* unlock_time */, priority, extra, false, m_trusted_daemon);
        break;
    }

    // if more than one tx necessary, prompt user to confirm
    if (m_wallet->always_confirm_transfers() || ptx_vector.size() > 1)
    {
        uint64_t total_sent = 0;
        uint64_t total_fee = 0;
        uint64_t dust_not_in_fee = 0;
        uint64_t dust_in_fee = 0;
        for (size_t n = 0; n < ptx_vector.size(); ++n)
        {
          total_fee += ptx_vector[n].fee;
          for (auto i: ptx_vector[n].selected_transfers)
            total_sent += m_wallet->get_transfer_details(i).amount();
          total_sent -= ptx_vector[n].change_dts.amount + ptx_vector[n].fee;

          if (ptx_vector[n].dust_added_to_fee)
            dust_in_fee += ptx_vector[n].dust;
          else
            dust_not_in_fee += ptx_vector[n].dust;
        }

        std::stringstream prompt;
        for (size_t n = 0; n < ptx_vector.size(); ++n)
        {
          prompt << tr("\nTransaction ") << (n + 1) << "/" << ptx_vector.size() << ":\n";
          subaddr_indices.clear();
          for (uint32_t i : ptx_vector[n].construction_data.subaddr_indices)
            subaddr_indices.insert(i);
          for (uint32_t i : subaddr_indices)
            prompt << boost::format(tr("Spending from address index %d\n")) % i;
          if (subaddr_indices.size() > 1)
            prompt << tr("WARNING: Outputs of multiple addresses are being used together, which might potentially compromise your privacy.\n");
        }
        prompt << boost::format(tr("Sending %s.  ")) % print_money(total_sent);
        if (ptx_vector.size() > 1)
        {
          prompt << boost::format(tr("Your transaction needs to be split into %llu transactions.  "
            "This will result in a transaction fee being applied to each transaction, for a total fee of %s")) %
            ((unsigned long long)ptx_vector.size()) % print_money(total_fee);
        }
        else
        {
          prompt << boost::format(tr("The transaction fee is %s")) %
            print_money(total_fee);
        }
        if (dust_in_fee != 0) prompt << boost::format(tr(", of which %s is dust from change")) % print_money(dust_in_fee);
        if (dust_not_in_fee != 0)  prompt << tr(".") << ENDL << boost::format(tr("A total of %s from dust change will be sent to dust address"))
                                                   % print_money(dust_not_in_fee);
        if (transfer_type == TransferLocked)
        {
					// Fixme: Need a better time expression in words
          float days = locked_blocks / 360.0f;
					float minutes = locked_blocks * 4.0f;
					prompt << boost::format(tr(".\nThis transaction will unlock on block %llu, in approximately %s minutes or %s days (assuming 4 minutes per block)")) % ((unsigned long long)unlock_block) % minutes % days;
        }
        prompt << tr(".") << ENDL << tr("Is this okay?  (Y/Yes/N/No): ");

        std::string accepted = command_line::input_line(prompt.str());
        if (std::cin.eof())
          return true;
        if (!command_line::is_yes(accepted))
        {
          fail_msg_writer() << tr("transaction cancelled.");

          // would like to return false, because no tx made, but everything else returns true
          // and I don't know what returning false might adversely affect.  *sigh*
          return true;
        }
    }

    // actually commit the transactions
    if (m_wallet->watch_only())
    {
      bool r = m_wallet->save_tx(ptx_vector, "unsigned_sumokoin_tx");
      if (!r)
      {
        fail_msg_writer() << tr("Failed to write transaction(s) to file");
      }
      else
      {
        success_msg_writer(true) << tr("Unsigned transaction(s) successfully written to file: ") << "unsigned_sumokoin_tx";
      }
    }
    else while (!ptx_vector.empty())
    {
      auto & ptx = ptx_vector.back();
      m_wallet->commit_tx(ptx);
      success_msg_writer(true) << tr("Money successfully sent, transaction ") << get_transaction_hash(ptx.tx);

      // if no exception, remove element from vector
      ptx_vector.pop_back();
    }
  }
  catch (const tools::error::daemon_busy&)
  {
    fail_msg_writer() << tr("daemon is busy. Please try again later.");
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    fail_msg_writer() << tr("no connection to daemon. Please make sure daemon is running.");
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("RPC error: " << e.to_string());
    fail_msg_writer() << tr("RPC error: ") << e.what();
  }
  catch (const tools::error::get_random_outs_error&)
  {
    fail_msg_writer() << tr("failed to get random outputs to mix");
  }
  catch (const tools::error::not_enough_money& e)
  {
    LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, sent amount %s") %
      print_money(e.available()) %
      print_money(e.tx_amount()));
    fail_msg_writer() << tr("Not enough money in unlocked balance");
  }
  catch (const tools::error::tx_not_possible& e)
  {
    LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)") %
      print_money(e.available()) %
      print_money(e.tx_amount() + e.fee())  %
      print_money(e.tx_amount()) %
      print_money(e.fee()));
    fail_msg_writer() << tr("Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees, or trying to send more money than the unlocked balance, or not leaving enough for fees");
  }
  catch (const tools::error::not_enough_outs_to_mix& e)
  {
    auto writer = fail_msg_writer();
    writer << tr("not enough outputs for specified mixin_count") << " = " << e.mixin_count() << ":";
    for (std::pair<uint64_t, uint64_t> outs_for_amount : e.scanty_outs())
    {
      writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.first) << ", " << tr("found outputs to mix") << " = " << outs_for_amount.second;
    }
  }
  catch (const tools::error::tx_not_constructed&)
  {
    fail_msg_writer() << tr("transaction was not constructed");
  }
  catch (const tools::error::tx_rejected& e)
  {
    fail_msg_writer() << (boost::format(tr("transaction %s was rejected by daemon with status: ")) % get_transaction_hash(e.tx())) << e.status();
    std::string reason = e.reason();
    if (!reason.empty())
      fail_msg_writer() << tr("Reason: ") << reason;
  }
  catch (const tools::error::tx_sum_overflow& e)
  {
    fail_msg_writer() << e.what();
  }
  catch (const tools::error::zero_destination&)
  {
    fail_msg_writer() << tr("one of destinations is zero");
  }
  catch (const tools::error::tx_too_big& e)
  {
    tx_size_target_factor = floorf((tx_size_target_factor * e.tx_size_limit()/get_object_blobsize(e.tx()))*100)/100;
    std::cout << boost::format(tr("constructed tx too big: tx size = %s bytes, limit = %s bytes; retrying with smaller tx_size_target_factor = %s...")) % get_object_blobsize(e.tx()) % e.tx_size_limit() % tx_size_target_factor << std::endl;
    retry = true;
  }
  catch (const tools::error::transfer_error& e)
  {
    LOG_ERROR("unknown transfer error: " << e.to_string());
    fail_msg_writer() << tr("unknown transfer error: ") << e.what();
  }
  catch (const tools::error::wallet_internal_error& e)
  {
    LOG_ERROR("internal error: " << e.to_string());
    fail_msg_writer() << tr("internal error: ") << e.what();
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    fail_msg_writer() << tr("unexpected error: ") << e.what();
  }
  catch (...)
  {
    LOG_ERROR("unknown error");
    fail_msg_writer() << tr("unknown error");
  }

  if (retry) transfer_main(transfer_type, args_, retry, tx_size_target_factor);

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::transfer(const std::vector<std::string> &args_)
{
  return transfer_main(TransferOriginal, args_);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::transfer_new(const std::vector<std::string> &args_)
{
  return transfer_main(TransferNew, args_);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::locked_transfer(const std::vector<std::string> &args_)
{
  return transfer_main(TransferLocked, args_);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sweep_all(const std::vector<std::string> &args_)
{
  return sweep_all(args_, false, 1.0f);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sweep_all(const std::vector<std::string> &args_, bool retry, float tx_size_target_factor)
{
  // sweep_all [index=<N1>[,<N2>,...]] [<mixin_count>] <address> [<payment_id>]
  if (!retry)
   if (!get_and_verify_password()) { return true; }

  if (!try_connect_to_daemon())
    return true;

  std::vector<std::string> local_args = args_;

  std::set<uint32_t> subaddr_indices;
  if (local_args.size() > 0 && local_args[0].substr(0, 6) == "index=")
  {
    if (!parse_subaddress_indices(local_args[0], subaddr_indices))
      return true;
    local_args.erase(local_args.begin());
  }

  size_t fake_outs_count;
  if(local_args.size() > 0) {
    if(!epee::string_tools::get_xtype_from_string(fake_outs_count, local_args[0]))
    {
      message_writer() << boost::format(tr("** No mixin value specified, default mixin %s will be used for this transaction.")) % (m_wallet->default_mixin() > 0 ? m_wallet->default_mixin() : DEFAULT_MIXIN);
			fake_outs_count = m_wallet->default_mixin() > 0 ? m_wallet->default_mixin() : DEFAULT_MIXIN;
    }
    else
    {
      if (fake_outs_count > MAX_MIXIN)
      {
        fail_msg_writer() << boost::format(tr("Given mixin value %s is too high. Max mixin value allowed is %s. Please resend tx with lower mixin.")) % fake_outs_count % MAX_MIXIN;
        return true;
      }

      if (fake_outs_count < DEFAULT_MIXIN)
			{
				std::stringstream prompt;
				prompt << boost::format(tr("Given mixin value %s is too low, default mixin %s will be used for this transaction. Is this okay?  (Y/Yes/N/No): ")) % fake_outs_count % (m_wallet->default_mixin() > 0 ? m_wallet->default_mixin() : DEFAULT_MIXIN);

				std::string accepted = command_line::input_line(prompt.str());
				if (std::cin.eof())
					return true;

				if (!command_line::is_yes(accepted))
				{
					fail_msg_writer() << tr("transaction cancelled.");
					return true;
				}

				fake_outs_count = m_wallet->default_mixin() > 0 ? m_wallet->default_mixin() : DEFAULT_MIXIN;
			}

      local_args.erase(local_args.begin());
    }
  }

  std::vector<uint8_t> extra;
  bool payment_id_seen = false;
  if (2 >= local_args.size())
  {
    std::string payment_id_str = local_args.back();

    crypto::hash payment_id;
    bool r = tools::wallet2::parse_long_payment_id(payment_id_str, payment_id);
    if(r)
    {
      std::string extra_nonce;
      set_payment_id_to_tx_extra_nonce(extra_nonce, payment_id);
      r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
      payment_id_seen = true;
    }
    else
    {
      crypto::hash8 payment_id8;
      r = tools::wallet2::parse_short_payment_id(payment_id_str, payment_id8);
      if(r)
      {
        std::string extra_nonce;
        set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, payment_id8);
        r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
        payment_id_seen = true;
      }
    }

    if (!r && local_args.size() == 3)
    {
      fail_msg_writer() << tr("payment id has invalid format, expected 16 or 64 character hex string: ") << payment_id_str;
      return true;
    }
    if (payment_id_seen)
      local_args.pop_back();
  }

  if (local_args.size() == 0)
  {
    fail_msg_writer() << tr("No address given");
    return true;
  }

  cryptonote::address_parse_info info;
  if (!cryptonote::get_account_address_from_str_or_url(info, m_wallet->testnet(), local_args[0]))
  {
    fail_msg_writer() << tr("failed to parse address");
    return true;
  }

  if (info.has_payment_id)
  {
    if (payment_id_seen)
    {
      fail_msg_writer() << tr("a single transaction cannot use more than one payment id: ") << local_args[0];
      return true;
    }

    std::string extra_nonce;
    set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, info.payment_id);
    bool r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
    if(!r)
    {
      fail_msg_writer() << tr("failed to set up payment id, though it was decoded correctly");
      return true;
    }
    payment_id_seen = true;
  }

  // prompt is there is no payment id and confirmation is required
  if (!retry && !payment_id_seen && m_wallet->confirm_missing_payment_id())
  {
     std::string accepted = command_line::input_line(tr("No payment id is included with this transaction. Is this okay?  (Y/Yes/N/No): "));
     if (std::cin.eof())
       return true;
     if (!command_line::is_yes(accepted))
     {
       fail_msg_writer() << tr("transaction cancelled.");

       // would like to return false, because no tx made, but everything else returns true
       // and I don't know what returning false might adversely affect.  *sigh*
       return true;
     }
  }

  retry = false;
  try
  {
    // figure out what tx will be necessary
    auto ptx_vector = m_wallet->create_transactions_all(0, info.address, fake_outs_count, 0 /* unlock_time */, 0 /* unused fee arg*/, extra, info.is_subaddress, m_current_subaddress_account, subaddr_indices, m_trusted_daemon, tx_size_target_factor);

    if (ptx_vector.empty())
    {
      fail_msg_writer() << tr("No outputs found");
      return true;
    }

    // give user total and fee, and prompt to confirm
    uint64_t total_fee = 0, total_sent = 0;
    for (size_t n = 0; n < ptx_vector.size(); ++n)
    {
      total_fee += ptx_vector[n].fee;
      for (auto i: ptx_vector[n].selected_transfers)
        total_sent += m_wallet->get_transfer_details(i).amount();
    }

    std::ostringstream prompt;
    for (size_t n = 0; n < ptx_vector.size(); ++n)
    {
      prompt << tr("\nTransaction ") << (n + 1) << "/" << ptx_vector.size() << ":\n";
      subaddr_indices.clear();
      for (uint32_t i : ptx_vector[n].construction_data.subaddr_indices)
        subaddr_indices.insert(i);
      for (uint32_t i : subaddr_indices)
        prompt << boost::format(tr("Spending from address index %d\n")) % i;
      if (subaddr_indices.size() > 1)
        prompt << tr("WARNING: Outputs of multiple addresses are being used together, which might potentially compromise your privacy.\n");
    }
    if (ptx_vector.size() > 1) {
      prompt << boost::format(tr("Sweeping %s in %llu transactions for a total fee of %s.  Is this okay?  (Y/Yes/N/No): ")) %
        print_money(total_sent) %
        ((unsigned long long)ptx_vector.size()) %
        print_money(total_fee);
    }
    else {
      prompt << boost::format(tr("Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No)")) %
        print_money(total_sent) %
        print_money(total_fee);
    }
    std::string accepted = command_line::input_line(prompt.str());
    if (std::cin.eof())
      return true;
    if (!command_line::is_yes(accepted))
    {
      fail_msg_writer() << tr("transaction cancelled.");

      // would like to return false, because no tx made, but everything else returns true
      // and I don't know what returning false might adversely affect.  *sigh*
      return true;
    }

    // actually commit the transactions
    if (m_wallet->watch_only())
    {
      bool r = m_wallet->save_tx(ptx_vector, "unsigned_sumokoin_tx");
      if (!r)
      {
        fail_msg_writer() << tr("Failed to write transaction(s) to file");
      }
      else
      {
        success_msg_writer(true) << tr("Unsigned transaction(s) successfully written to file: ") << "unsigned_sumokoin_tx";
      }
    }
    else while (!ptx_vector.empty())
    {
      auto & ptx = ptx_vector.back();
      m_wallet->commit_tx(ptx);
      success_msg_writer(true) << tr("Money successfully sent, transaction: ") << get_transaction_hash(ptx.tx);

      // if no exception, remove element from vector
      ptx_vector.pop_back();
    }
  }
  catch (const tools::error::daemon_busy&)
  {
    fail_msg_writer() << tr("daemon is busy. Please try again later.");
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    fail_msg_writer() << tr("no connection to daemon. Please make sure daemon is running.");
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("RPC error: " << e.to_string());
    fail_msg_writer() << tr("RPC error: ") << e.what();
  }
  catch (const tools::error::get_random_outs_error&)
  {
    fail_msg_writer() << tr("failed to get random outputs to mix");
  }
  catch (const tools::error::not_enough_money& e)
  {
    LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, sent amount %s") %
      print_money(e.available()) %
      print_money(e.tx_amount()));
    fail_msg_writer() << tr("Not enough money in unlocked balance");
  }
  catch (const tools::error::tx_not_possible& e)
  {
    LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)") %
      print_money(e.available()) %
      print_money(e.tx_amount() + e.fee())  %
      print_money(e.tx_amount()) %
      print_money(e.fee()));
    fail_msg_writer() << tr("Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees, or trying to send more money than the unlocked balance, or not leaving enough for fees");
  }
  catch (const tools::error::not_enough_outs_to_mix& e)
  {
    auto writer = fail_msg_writer();
    writer << tr("not enough outputs for specified mixin_count") << " = " << e.mixin_count() << ":";
    for (std::pair<uint64_t, uint64_t> outs_for_amount : e.scanty_outs())
    {
      writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.first) << ", " << tr("found outputs to mix") << " = " << outs_for_amount.second;
    }
  }
  catch (const tools::error::tx_not_constructed&)
  {
    fail_msg_writer() << tr("transaction was not constructed");
  }
  catch (const tools::error::tx_rejected& e)
  {
    fail_msg_writer() << (boost::format(tr("transaction %s was rejected by daemon with status: ")) % get_transaction_hash(e.tx())) << e.status();
    std::string reason = e.reason();
    if (!reason.empty())
      fail_msg_writer() << tr("Reason: ") << reason;
  }
  catch (const tools::error::tx_sum_overflow& e)
  {
    fail_msg_writer() << e.what();
  }
  catch (const tools::error::zero_destination&)
  {
    fail_msg_writer() << tr("one of destinations is zero");
  }
  catch (const tools::error::tx_too_big& e)
  {
    tx_size_target_factor = floorf((tx_size_target_factor * e.tx_size_limit()/get_object_blobsize(e.tx()))*100)/100;
    std::cout << boost::format(tr("constructed tx too big: tx size = %s bytes, limit = %s bytes; retrying with smaller tx_size_target_factor = %s...")) % get_object_blobsize(e.tx()) % e.tx_size_limit() % tx_size_target_factor << std::endl;
    retry = true;
  }
  catch (const tools::error::transfer_error& e)
  {
    LOG_ERROR("unknown transfer error: " << e.to_string());
    fail_msg_writer() << tr("unknown transfer error: ") << e.what();
  }
  catch (const tools::error::wallet_internal_error& e)
  {
    LOG_ERROR("internal error: " << e.to_string());
    fail_msg_writer() << tr("internal error: ") << e.what();
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    fail_msg_writer() << tr("unexpected error: ") << e.what();
  }
  catch (...)
  {
    LOG_ERROR("unknown error");
    fail_msg_writer() << tr("unknown error");
  }

  if (retry) sweep_all(args_, retry, tx_size_target_factor);

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::accept_loaded_tx(const std::function<size_t()> get_num_txes, const std::function<const tools::wallet2::tx_construction_data&(size_t)> &get_tx, const std::string &extra_message)
{
  // gather info to ask the user
  uint64_t amount = 0, amount_to_dests = 0, change = 0;
  size_t min_mixin = ~0;
  std::unordered_map<cryptonote::account_public_address, std::pair<std::string, uint64_t>> dests;
  const std::string wallet_address = m_wallet->get_account().get_public_address_str(m_wallet->testnet());
  for (size_t n = 0; n < get_num_txes(); ++n)
  {
    const tools::wallet2::tx_construction_data &cd = get_tx(n);
    for (size_t s = 0; s < cd.sources.size(); ++s)
    {
      amount += cd.sources[s].amount;
      size_t mixin = cd.sources[s].outputs.size() - 1;
      if (mixin < min_mixin)
        min_mixin = mixin;
    }
    for (size_t d = 0; d < cd.splitted_dsts.size(); ++d)
    {
      const tx_destination_entry &entry = cd.splitted_dsts[d];
      std::string address, standard_address = get_account_address_as_str(m_wallet->testnet(), entry.is_subaddress, entry.addr);
      auto i = dests.find(entry.addr);
      if (i == dests.end())
        dests.insert(std::make_pair(entry.addr, std::make_pair(address, entry.amount)));
      else
        i->second.second += entry.amount;
      amount_to_dests += entry.amount;
    }
    if (cd.change_dts.amount > 0)
    {
      auto it = dests.find(cd.change_dts.addr);
      if (it == dests.end())
      {
        fail_msg_writer() << tr("Claimed change does not go to a paid address");
        return false;
      }
      if (it->second.second < cd.change_dts.amount)
      {
        fail_msg_writer() << tr("Claimed change is larger than payment to the change address");
        return false;
      }
      if (memcmp(&cd.change_dts.addr, &get_tx(0).change_dts.addr, sizeof(cd.change_dts.addr)))
      {
        fail_msg_writer() << tr("Change does to more than one address");
        return false;
      }
      change += cd.change_dts.amount;
      it->second.second -= cd.change_dts.amount;
      if (it->second.second == 0)
        dests.erase(cd.change_dts.addr);
    }
  }
  std::string dest_string;
  for (auto i = dests.begin(); i != dests.end();)
  {
    dest_string += (boost::format(tr("sending %s to %s")) % print_money(i->second.second) % i->second.first).str();
    ++i;
    if (i != dests.end())
      dest_string += ", ";
  }
  if (dest_string.empty())
    dest_string = tr("with no destinations");

  std::string change_string;
  if (change > 0)
  {
    std::string address = get_account_address_as_str(m_wallet->testnet(), get_tx(0).subaddr_account > 0, get_tx(0).change_dts.addr);
    change_string += (boost::format(tr("%s change to %s")) % print_money(change) % address).str();
  }
  else
    change_string += tr("no change");

  uint64_t fee = amount - amount_to_dests;
  std::string prompt_str = (boost::format(tr("Loaded %lu transactions, for %s, fee %s, %s, %s, with min mixin %lu. %sIs this okay? (Y/Yes/N/No): ")) % (unsigned long)get_num_txes() % print_money(amount) % print_money(fee) % dest_string % change_string % (unsigned long)min_mixin % extra_message).str();
  return command_line::is_yes(command_line::input_line(prompt_str));
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::accept_loaded_tx(const tools::wallet2::unsigned_tx_set &txs)
{
  std::string extra_message;
  if (!txs.transfers.empty())
    extra_message = (boost::format("%u outputs to import. ") % (unsigned)txs.transfers.size()).str();
  return accept_loaded_tx([&txs](){return txs.txes.size();}, [&txs](size_t n)->const tools::wallet2::tx_construction_data&{return txs.txes[n];}, extra_message);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::accept_loaded_tx(const tools::wallet2::signed_tx_set &txs)
{
  std::string extra_message;
  if (!txs.key_images.empty())
    extra_message = (boost::format("%u key images to import. ") % (unsigned)txs.key_images.size()).str();
  return accept_loaded_tx([&txs](){return txs.ptx.size();}, [&txs](size_t n)->const tools::wallet2::tx_construction_data&{return txs.ptx[n].construction_data;}, extra_message);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sign_transfer(const std::vector<std::string> &args_)
{
  if(m_wallet->watch_only())
  {
     fail_msg_writer() << tr("This is a watch only wallet");
     return true;
  }

  if (!get_and_verify_password()) { return true; }

  std::vector<tools::wallet2::pending_tx> ptx;
  try
  {
    bool r = m_wallet->sign_tx("unsigned_sumokoin_tx", "signed_sumokoin_tx", ptx, [&](const tools::wallet2::unsigned_tx_set &tx){ return accept_loaded_tx(tx); });
    if (!r)
    {
      fail_msg_writer() << tr("Failed to sign transaction");
      return true;
    }
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << tr("Failed to sign transaction: ") << e.what();
    return true;
  }

  std::string txids_as_text;
  for (const auto &t: ptx)
  {
    if (!txids_as_text.empty())
      txids_as_text += (", ");
    txids_as_text += epee::string_tools::pod_to_hex(get_transaction_hash(t.tx));
  }
  success_msg_writer(true) << tr("Transaction successfully signed to file ") << "signed_sumokoin_tx" << ", txid " << txids_as_text;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::submit_transfer(const std::vector<std::string> &args_)
{
  if (!try_connect_to_daemon())
    return true;

  try
  {
    std::vector<tools::wallet2::pending_tx> ptx_vector;
    bool r = m_wallet->load_tx("signed_sumokoin_tx", ptx_vector, [&](const tools::wallet2::signed_tx_set &tx){ return accept_loaded_tx(tx); });
    if (!r)
    {
      fail_msg_writer() << tr("Failed to load transaction from file");
      return true;
    }

    // actually commit the transactions
    while (!ptx_vector.empty())
    {
      auto & ptx = ptx_vector.back();
      m_wallet->commit_tx(ptx);
      success_msg_writer(true) << tr("Money successfully sent, transaction: ") << get_transaction_hash(ptx.tx);

      // if no exception, remove element from vector
      ptx_vector.pop_back();
    }
  }
  catch (const tools::error::daemon_busy&)
  {
    fail_msg_writer() << tr("daemon is busy. Please try later");
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    fail_msg_writer() << tr("no connection to daemon. Please, make sure daemon is running.");
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("Unknown RPC error: " << e.to_string());
    fail_msg_writer() << tr("RPC error: ") << e.what();
  }
  catch (const tools::error::get_random_outs_error&)
  {
    fail_msg_writer() << tr("failed to get random outputs to mix");
  }
  catch (const tools::error::not_enough_money& e)
  {
    LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, sent amount %s") %
      print_money(e.available()) %
      print_money(e.tx_amount()));
    fail_msg_writer() << tr("Not enough money in unlocked balance");
  }
  catch (const tools::error::tx_not_possible& e)
  {
    LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)") %
      print_money(e.available()) %
      print_money(e.tx_amount() + e.fee())  %
      print_money(e.tx_amount()) %
      print_money(e.fee()));
    fail_msg_writer() << tr("Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees, or trying to send more money than the unlocked balance, or not leaving enough for fees");
  }
  catch (const tools::error::not_enough_outs_to_mix& e)
  {
    auto writer = fail_msg_writer();
    writer << tr("not enough outputs for specified mixin_count") << " = " << e.mixin_count() << ":";
    for (std::pair<uint64_t, uint64_t> outs_for_amount : e.scanty_outs())
    {
      writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.first) << ", " << tr("found outputs to mix") << " = " << outs_for_amount.second;
    }
  }
  catch (const tools::error::tx_not_constructed&)
  {
    fail_msg_writer() << tr("transaction was not constructed");
  }
  catch (const tools::error::tx_rejected& e)
  {
    fail_msg_writer() << (boost::format(tr("transaction %s was rejected by daemon with status: ")) % get_transaction_hash(e.tx())) << e.status();
  }
  catch (const tools::error::tx_sum_overflow& e)
  {
    fail_msg_writer() << e.what();
  }
  catch (const tools::error::zero_destination&)
  {
    fail_msg_writer() << tr("one of destinations is zero");
  }
  catch (const tools::error::tx_too_big& e)
  {
    fail_msg_writer() << tr("Failed to find a suitable way to split transactions");
  }
  catch (const tools::error::transfer_error& e)
  {
    LOG_ERROR("unknown transfer error: " << e.to_string());
    fail_msg_writer() << tr("unknown transfer error: ") << e.what();
  }
  catch (const tools::error::wallet_internal_error& e)
  {
    LOG_ERROR("internal error: " << e.to_string());
    fail_msg_writer() << tr("internal error: ") << e.what();
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    fail_msg_writer() << tr("unexpected error: ") << e.what();
  }
  catch (...)
  {
    LOG_ERROR("Unknown error");
    fail_msg_writer() << tr("unknown error");
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::get_tx_key(const std::vector<std::string> &args_)
{
  std::vector<std::string> local_args = args_;

  if(local_args.size() != 1) {
    fail_msg_writer() << tr("usage: get_tx_key <txid>");
    return true;
  }
  if (!get_and_verify_password()) { return true; }

  cryptonote::blobdata txid_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(local_args.front(), txid_data))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return false;
  }
  crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

  LOCK_IDLE_SCOPE();

  crypto::secret_key tx_key;
  std::vector<crypto::secret_key> amount_keys;
  if (m_wallet->get_tx_key(txid, tx_key))
  {
    success_msg_writer() << tr("Tx key: ") << epee::string_tools::pod_to_hex(tx_key);
    return true;
  }
  else
  {
    fail_msg_writer() << tr("no tx keys found for this txid");
    return true;
  }
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::check_tx_key(const std::vector<std::string> &args_)
{
  std::vector<std::string> local_args = args_;

  if(local_args.size() != 3) {
    fail_msg_writer() << tr("usage: check_tx_key <txid> <txkey> <address>");
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  assert(m_wallet);
  cryptonote::blobdata txid_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(local_args[0], txid_data))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return true;
  }
  crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

  LOCK_IDLE_SCOPE();

  if (local_args[1].size() < 64 || local_args[1].size() % 64)
  {
    fail_msg_writer() << tr("failed to parse tx key");
    return true;
  }
  crypto::secret_key tx_key;
  cryptonote::blobdata tx_key_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(local_args[1], tx_key_data))
  {
    fail_msg_writer() << tr("failed to parse tx key");
    return true;
  }
  tx_key = *reinterpret_cast<const crypto::secret_key*>(tx_key_data.data());

  cryptonote::address_parse_info info;
  if (!cryptonote::get_account_address_from_str_or_url(info, m_wallet->testnet(), local_args[2]))
  {
    fail_msg_writer() << tr("failed to parse address");
    return true;
  }

  COMMAND_RPC_GET_TRANSACTIONS::request req;
  COMMAND_RPC_GET_TRANSACTIONS::response res;
  req.txs_hashes.push_back(epee::string_tools::pod_to_hex(txid));
  if (!net_utils::invoke_http_json_remote_command2(m_wallet->get_daemon_address() + "/gettransactions", req, res, m_http_client) ||
      (res.txs.size() != 1 && res.txs_as_hex.size() != 1))
  {
    fail_msg_writer() << tr("failed to get transaction from daemon");
    return true;
  }
  cryptonote::blobdata tx_data;
  bool ok;
  if (res.txs.size() == 1)
    ok = string_tools::parse_hexstr_to_binbuff(res.txs.front().as_hex, tx_data);
  else
    ok = string_tools::parse_hexstr_to_binbuff(res.txs_as_hex.front(), tx_data);
  if (!ok)
  {
    fail_msg_writer() << tr("failed to parse transaction from daemon");
    return true;
  }
  crypto::hash tx_hash, tx_prefix_hash;
  cryptonote::transaction tx;
  if (!cryptonote::parse_and_validate_tx_from_blob(tx_data, tx, tx_hash, tx_prefix_hash))
  {
    fail_msg_writer() << tr("failed to validate transaction from daemon");
    return true;
  }
  if (tx_hash != txid)
  {
    fail_msg_writer() << tr("failed to get the right transaction from daemon");
    return true;
  }

  crypto::key_derivation derivation;
  if (!crypto::generate_key_derivation(info.address.m_view_public_key, tx_key, derivation))
  {
    fail_msg_writer() << tr("failed to generate key derivation from supplied parameters");
    return true;
  }

  uint64_t received = 0;
  try {
    for (size_t n = 0; n < tx.vout.size(); ++n)
    {
      if (typeid(txout_to_key) != tx.vout[n].target.type())
        continue;
      const txout_to_key tx_out_to_key = boost::get<txout_to_key>(tx.vout[n].target);
      crypto::public_key pubkey;
      derive_public_key(derivation, n, info.address.m_spend_public_key, pubkey);
      if (pubkey == tx_out_to_key.key)
      {
        uint64_t amount;
        if (tx.version == 1)
        {
          amount = tx.vout[n].amount;
        }
        else
        {
          try
          {
            rct::key Ctmp;
            //rct::key amount_key = rct::hash_to_scalar(rct::scalarmultKey(rct::pk2rct(address.m_view_public_key), rct::sk2rct(tx_key)));
            crypto::key_derivation derivation;
            bool r = crypto::generate_key_derivation(info.address.m_view_public_key, tx_key, derivation);
            if (!r)
            {
              LOG_ERROR("Failed to generate key derivation to decode rct output " << n);
              amount = 0;
            }
            else
            {
              crypto::secret_key scalar1;
              crypto::derivation_to_scalar(derivation, n, scalar1);
              rct::ecdhTuple ecdh_info = tx.rct_signatures.ecdhInfo[n];
              rct::ecdhDecode(ecdh_info, rct::sk2rct(scalar1));
              rct::key C = tx.rct_signatures.outPk[n].mask;
              rct::addKeys2(Ctmp, ecdh_info.mask, ecdh_info.amount, rct::H);
              if (rct::equalKeys(C, Ctmp))
                amount = rct::h2d(ecdh_info.amount);
              else
                amount = 0;
            }
          }
          catch (...) { amount = 0; }
        }
        received += amount;
      }
    }
  }
  catch(const std::exception &e)
  {
    LOG_ERROR("error: " << e.what());
    fail_msg_writer() << tr("error: ") << e.what();
    return true;
  }

  std::string address_str = get_account_address_as_str(m_wallet->testnet(), info.is_subaddress, info.address);
  if (received > 0)
  {
    success_msg_writer() << address_str << " " << tr("received") << " " << print_money(received) << " " << tr("in txid") << " " << txid;
  }
  else
  {
    fail_msg_writer() << address_str << " " << tr("received nothing in txid") << " " << txid;
  }
  if (res.txs.front().in_pool)
  {
    success_msg_writer() << tr("WARNING: this transaction is not yet included in the blockchain!");
  }
  else
  {
    std::string err;
    uint64_t bc_height = get_daemon_blockchain_height(err);
    if (err.empty())
    {
      uint64_t confirmations = bc_height - (res.txs.front().block_height + 1);
      success_msg_writer() << boost::format(tr("This transaction has %u confirmations")) % confirmations;
    }
    else
    {
      success_msg_writer() << tr("WARNING: failed to determine number of confirmations!");
    }
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
static std::string get_human_readable_timestamp(uint64_t ts)
{
  char buffer[64];
  if (ts < 1234567890)
    return "<unknown>";
  time_t tt = ts;
  struct tm tm;
#ifdef WIN32
  gmtime_s(&tm, &tt);
#else
  gmtime_r(&tt, &tm);
#endif
  uint64_t now = time(NULL);
  uint64_t diff = ts > now ? ts - now : now - ts;
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
  return std::string(buffer);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_transfers(const std::vector<std::string> &args_)
{
  std::vector<std::string> local_args = args_;
  bool in = true;
  bool out = true;
  bool pending = true;
  bool failed = true;
  bool pool = true;
  uint64_t min_height = 0;
  uint64_t max_height = (uint64_t)-1;
  boost::optional<uint32_t> subaddr_index;

  if (local_args.size() > 4) {
    fail_msg_writer() << tr("usage: show_transfers [in|out|all|pending|pool|failed] [index=<N1>[,<N2>,...]] [<min_height> [<max_height>]]");
    return true;
  }

  LOCK_IDLE_SCOPE();

  // optional in/out selector
  if (local_args.size() > 0) {
    if (local_args[0] == "in" || local_args[0] == "incoming") {
      out = pending = failed = false;
      local_args.erase(local_args.begin());
    }
    else if (local_args[0] == "out" || local_args[0] == "outgoing") {
      in = pool = false;
      local_args.erase(local_args.begin());
    }
    else if (local_args[0] == "pending") {
      in = out = failed = false;
      local_args.erase(local_args.begin());
    }
    else if (local_args[0] == "failed") {
      in = out = pending = pool = false;
      local_args.erase(local_args.begin());
    }
    else if (local_args[0] == "pool") {
      in = out = pending = failed = false;
      local_args.erase(local_args.begin());
    }
    else if (local_args[0] == "all" || local_args[0] == "both") {
      local_args.erase(local_args.begin());
    }
  }

  // subaddr_index
  std::set<uint32_t> subaddr_indices;
  if (local_args.size() > 0 && local_args[0].substr(0, 6) == "index=")
  {
    if (!parse_subaddress_indices(local_args[0], subaddr_indices))
      return true;
    local_args.erase(local_args.begin());
  }

  // min height
  if (local_args.size() > 0) {
    try {
      min_height = boost::lexical_cast<uint64_t>(local_args[0]);
    }
    catch (boost::bad_lexical_cast &) {
      fail_msg_writer() << tr("bad min_height parameter:") << " " << local_args[0];
      return true;
    }
    local_args.erase(local_args.begin());
  }

  // max height
  if (local_args.size() > 0) {
    try {
      max_height = boost::lexical_cast<uint64_t>(local_args[0]);
    }
    catch (boost::bad_lexical_cast &) {
      fail_msg_writer() << tr("bad max_height parameter:") << " " << local_args[0];
      return true;
    }
    local_args.erase(local_args.begin());
  }

  std::multimap<uint64_t, std::pair<bool,std::string>> output;

  if (in) {
    std::list<std::pair<crypto::hash, tools::wallet2::payment_details>> payments;
    m_wallet->get_payments(payments, min_height, max_height, m_current_subaddress_account, subaddr_indices);
    for (std::list<std::pair<crypto::hash, tools::wallet2::payment_details>>::const_iterator i = payments.begin(); i != payments.end(); ++i) {
      const tools::wallet2::payment_details &pd = i->second;
      std::string payment_id = string_tools::pod_to_hex(i->first);
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      std::string note = m_wallet->get_tx_note(pd.m_tx_hash);
      output.insert(std::make_pair(pd.m_block_height, std::make_pair(true, (boost::format("%25.25s %20.20s %s %s %d %s %s") % get_human_readable_timestamp(pd.m_timestamp) % print_money(pd.m_amount) % string_tools::pod_to_hex(pd.m_tx_hash) % payment_id % pd.m_subaddr_index.minor % "-" % note).str())));
    }
  }

  auto print_subaddr_indices = [](const std::set<uint32_t>& indices)
  {
    stringstream ss;
    bool first = true;
    for (uint32_t i : indices)
    {
      ss << (first ? "" : ",") << i;
      first = false;
    }
    return ss.str();
  };

  if (out) {
    std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>> payments;
    m_wallet->get_payments_out(payments, min_height, max_height, m_current_subaddress_account, subaddr_indices);
    for (std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>>::const_iterator i = payments.begin(); i != payments.end(); ++i) {
      const tools::wallet2::confirmed_transfer_details &pd = i->second;
      uint64_t change = pd.m_change == (uint64_t)-1 ? 0 : pd.m_change; // change may not be known
      uint64_t fee = pd.m_amount_in - pd.m_amount_out;
      std::string dests;
      for (const auto &d: pd.m_dests) {
        if (!dests.empty())
          dests += ", ";
        dests += get_account_address_as_str(m_wallet->testnet(), pd.m_dest_subaddr, d.addr) + ": " + print_money(d.amount);
      }
      std::string payment_id = string_tools::pod_to_hex(i->second.m_payment_id);
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      std::string note = m_wallet->get_tx_note(i->first);
      output.insert(std::make_pair(pd.m_block_height, std::make_pair(false, (boost::format("%25.25s %20.20s %s %s %14.14s %s %s - %s") % get_human_readable_timestamp(pd.m_timestamp) % print_money(pd.m_amount_in - change - fee) % string_tools::pod_to_hex(i->first) % payment_id % print_money(fee) % dests % print_subaddr_indices(pd.m_subaddr_indices) % note).str())));
    }
  }

  // print in and out sorted by height
  for (std::map<uint64_t, std::pair<bool, std::string>>::const_iterator i = output.begin(); i != output.end(); ++i) {
    message_writer(i->second.first ? epee::log_space::console_color_green : epee::log_space::console_color_magenta, false) <<
      boost::format("%8.8llu %6.6s %s") %
      ((unsigned long long)i->first) % (i->second.first ? tr("in") : tr("out")) % i->second.second;
  }

  if (pool) {
    try
    {
      m_wallet->update_pool_state();
      std::list<std::pair<crypto::hash, tools::wallet2::payment_details>> payments;
      m_wallet->get_unconfirmed_payments(payments, m_current_subaddress_account, subaddr_indices);
      for (std::list<std::pair<crypto::hash, tools::wallet2::payment_details>>::const_iterator i = payments.begin(); i != payments.end(); ++i) {
        const tools::wallet2::payment_details &pd = i->second;
        std::string payment_id = string_tools::pod_to_hex(i->first);
        if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
          payment_id = payment_id.substr(0,16);
        std::string note = m_wallet->get_tx_note(pd.m_tx_hash);
        message_writer() << (boost::format("%8.8s %6.6s %25.25s %20.20s %s %s %d %s %s") % "pool" % "in" % get_human_readable_timestamp(pd.m_timestamp) % print_money(pd.m_amount) % string_tools::pod_to_hex(pd.m_tx_hash) % payment_id % pd.m_subaddr_index.minor % "-" % note).str();
      }
    }
    catch (const std::exception& e)
    {
      fail_msg_writer() << "Failed to get pool state:" << e.what();
    }
  }

  // print unconfirmed last
  if (pending || failed) {
    std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>> upayments;
    m_wallet->get_unconfirmed_payments_out(upayments, m_current_subaddress_account, subaddr_indices);
    for (std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>>::const_iterator i = upayments.begin(); i != upayments.end(); ++i) {
      const tools::wallet2::unconfirmed_transfer_details &pd = i->second;
      uint64_t amount = pd.m_amount_in;
      uint64_t fee = amount - pd.m_amount_out;
      std::string payment_id = string_tools::pod_to_hex(i->second.m_payment_id);
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      std::string note = m_wallet->get_tx_note(i->first);
      bool is_failed = pd.m_state == tools::wallet2::unconfirmed_transfer_details::failed;
      if ((failed && is_failed) || (!is_failed && pending)) {
        message_writer() << (boost::format("%8.8s %6.6s %25.25s %20.20s %s %s %14.14s %s - %s") % (is_failed ? tr("failed") : tr("pending")) % tr("out") % get_human_readable_timestamp(pd.m_timestamp) % print_money(amount - pd.m_change - fee) % string_tools::pod_to_hex(i->first) % payment_id % print_money(fee) % print_subaddr_indices(pd.m_subaddr_indices) % note).str();
      }
    }
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::rescan_blockchain(const std::vector<std::string> &args_)
{
  return refresh_main(0, true);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::wallet_idle_thread()
{
  while (true)
  {
    boost::unique_lock<boost::mutex> lock(m_idle_mutex);
    if (!m_idle_run.load(std::memory_order_relaxed))
      break;

    // auto refresh
    if (m_auto_refresh_enabled)
    {
      m_auto_refresh_refreshing = true;
      try
      {
        uint64_t fetched_blocks;
        if (try_connect_to_daemon(true))
          m_wallet->refresh(0, fetched_blocks);
      }
      catch(...) {}
      m_auto_refresh_refreshing = false;
    }

    if (!m_idle_run.load(std::memory_order_relaxed))
      break;
    m_idle_cond.wait_for(lock, boost::chrono::seconds(90));
  }
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::run()
{
  // check and display warning, but go on anyway
  try_connect_to_daemon();

  refresh_main(0, false, true);

  m_auto_refresh_enabled = m_wallet->auto_refresh();
  m_idle_thread = boost::thread([&]{wallet_idle_thread();});

  std::string addr_start = m_wallet->get_subaddress_as_str({ m_current_subaddress_account, 0 }).substr(0, 9);
  message_writer(epee::log_space::console_color_green, false) << "Background refresh thread started";
  std::ostringstream prompt;
  prompt << "[" << tr("wallet/") << m_current_subaddress_account << " " << addr_start << "]: ";
  return m_cmd_binder.run_handling(prompt.str(), "");
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::stop()
{
  m_cmd_binder.stop_handling();

  m_idle_run.store(false, std::memory_order_relaxed);
  m_wallet->stop();
  // make the background refresh thread quit
  boost::unique_lock<boost::mutex> lock(m_idle_mutex);
  m_idle_cond.notify_one();
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::account(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  // Usage:
  //   account
  //   account new <label text with white spaces allowed>
  //   account switch <index>
  //   account label <index> <label text with white spaces allowed>

  if (args.empty())
  {
    // print all the existing accounts
    LOCK_IDLE_SCOPE();
    print_accounts();
    return true;
  }

  std::vector<std::string> local_args = args;
  std::string command = local_args[0];
  local_args.erase(local_args.begin());
  if (command == "new")
  {
    // create a new account and switch to it
    std::string label = boost::join(local_args, " ");
    if (label.empty())
      label = tr("(Untitled account)");
    m_wallet->add_subaddress_account(label);
    m_current_subaddress_account = m_wallet->get_num_subaddress_accounts() - 1;
    //update_prompt();
    LOCK_IDLE_SCOPE();
    print_accounts();
  }
  else if (command == "switch" && local_args.size() == 1)
  {
    // switch to the specified account
    uint32_t index_major;
    if (!epee::string_tools::get_xtype_from_string(index_major, local_args[0]))
    {
      fail_msg_writer() << tr("failed to parse index: ") << local_args[0];
      return true;
    }
    if (index_major >= m_wallet->get_num_subaddress_accounts())
    {
      fail_msg_writer() << tr("specify an index between 0 and ") << (m_wallet->get_num_subaddress_accounts() - 1);
      return true;
    }
    m_current_subaddress_account = index_major;
    //update_prompt();
    show_balance();
  }
  else if (command == "label" && local_args.size() >= 1)
  {
    // set label of the specified account
    uint32_t index_major;
    if (!epee::string_tools::get_xtype_from_string(index_major, local_args[0]))
    {
      fail_msg_writer() << tr("failed to parse index: ") << local_args[0];
      return true;
    }
    local_args.erase(local_args.begin());
    std::string label = boost::join(local_args, " ");
    try
    {
      m_wallet->set_subaddress_label({ index_major, 0 }, label);
      LOCK_IDLE_SCOPE();
      print_accounts();
    }
    catch (const std::exception& e)
    {
      fail_msg_writer() << e.what();
    }
  }
  else
  {
    fail_msg_writer() << tr("usage: account [new <label text with white spaces allowed> | switch <index> | label <index> <label text with white spaces allowed>]");
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::update_prompt()
{
  std::ostringstream prompt;
  prompt << "[" << tr("wallet/") << m_current_subaddress_account << " " << m_wallet->get_subaddress_as_str({ m_current_subaddress_account, 0 }).substr(0, 9) << "]: ";
  m_cmd_binder.set_prompt(prompt.str());
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::print_accounts()
{
  success_msg_writer() << boost::format("%18s %21s %21s %21s") % tr("Account") % tr("Balance") % tr("Unlocked balance") % tr("Label");
  for (uint32_t account_index = 0; account_index < m_wallet->get_num_subaddress_accounts(); ++account_index)
  {
    success_msg_writer() << boost::format(tr("%8u %9s %21s %21s %21s"))
      % account_index
      % m_wallet->get_subaddress_as_str({ account_index, 0 }).substr(0, 9)
      % print_money(m_wallet->balance(account_index))
      % print_money(m_wallet->unlocked_balance(account_index))
      % m_wallet->get_subaddress_label({ account_index, 0 }).substr(0, 21);
  }
  success_msg_writer() << tr("-------------------------------------------------------------------------------------");
  success_msg_writer() << boost::format(tr("%18s %21s %21s")) % "Total" % print_money(m_wallet->balance_all()) % print_money(m_wallet->unlocked_balance_all());
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::print_address(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  // Usage:
  //  address
  //  address new <label text with white spaces allowed>
  //  address all
  //  address <index_min> [<index_max>]
  //  address label <index> <label text with white spaces allowed>

  std::vector<std::string> local_args = args;
  tools::wallet2::transfer_container transfers;
  m_wallet->get_transfers(transfers);

  auto print_address_sub = [this, &transfers](uint32_t index)
  {
    bool used = std::find_if(
      transfers.begin(), transfers.end(),
      [this, &index](const tools::wallet2::transfer_details& td) {
      return td.m_subaddr_index == cryptonote::subaddress_index{ m_current_subaddress_account, index };
    }) != transfers.end();
    success_msg_writer() << index << "  " << m_wallet->get_subaddress_as_str({ m_current_subaddress_account, index }) << "  " << (index == 0 ? tr("Primary address") : m_wallet->get_subaddress_label({ m_current_subaddress_account, index })) << " " << (used ? tr("(used)") : "");
  };

  uint32_t index = 0;
  if (local_args.empty())
  {
    print_address_sub(index);
  }
  else if (local_args.size() == 1 && local_args[0] == "all")
  {
    local_args.erase(local_args.begin());
    for (; index < m_wallet->get_num_subaddresses(m_current_subaddress_account); ++index)
      print_address_sub(index);
  }
  else if (local_args[0] == "new")
  {
    local_args.erase(local_args.begin());
    std::string label;
    if (local_args.size() > 0)
      label = boost::join(local_args, " ");
    if (label.empty())
      label = tr("(Untitled address)");
    m_wallet->add_subaddress(m_current_subaddress_account, label);
    print_address_sub(m_wallet->get_num_subaddresses(m_current_subaddress_account) - 1);
  }
  else if (local_args.size() >= 2 && local_args[0] == "label")
  {
    if (!epee::string_tools::get_xtype_from_string(index, local_args[1]))
    {
      fail_msg_writer() << tr("failed to parse index: ") << local_args[1];
      return true;
    }
    if (index >= m_wallet->get_num_subaddresses(m_current_subaddress_account))
    {
      fail_msg_writer() << tr("specify an index between 0 and ") << (m_wallet->get_num_subaddresses(m_current_subaddress_account) - 1);
      return true;
    }
    local_args.erase(local_args.begin());
    local_args.erase(local_args.begin());
    std::string label = boost::join(local_args, " ");
    m_wallet->set_subaddress_label({ m_current_subaddress_account, index }, label);
    print_address_sub(index);
  }
  else if (local_args.size() <= 2 && epee::string_tools::get_xtype_from_string(index, local_args[0]))
  {
    local_args.erase(local_args.begin());
    uint32_t index_min = index;
    uint32_t index_max = index_min;
    if (local_args.size() > 0)
    {
      if (!epee::string_tools::get_xtype_from_string(index_max, local_args[0]))
      {
        fail_msg_writer() << tr("failed to parse index: ") << local_args[0];
        return true;
      }
      local_args.erase(local_args.begin());
    }
    if (index_max < index_min)
      std::swap(index_min, index_max);
    if (index_min >= m_wallet->get_num_subaddresses(m_current_subaddress_account))
    {
      fail_msg_writer() << tr("<index_min> is already out of bound");
      return true;
    }
    if (index_max >= m_wallet->get_num_subaddresses(m_current_subaddress_account))
    {
      message_writer() << tr("<index_max> exceeds the bound");
      index_max = m_wallet->get_num_subaddresses(m_current_subaddress_account) - 1;
    }
    for (index = index_min; index <= index_max; ++index)
      print_address_sub(index);
  }
  else
  {
    fail_msg_writer() << tr("usage: address [ new <label text with white spaces allowed> | all | <index_min> [<index_max>] | label <index> <label text with white spaces allowed> ]");
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::print_integrated_address(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  crypto::hash8 payment_id;
  if (args.size() > 2)
  {
    fail_msg_writer() << tr("usage: integrated_address [<index>] [<payment_id>]");
    return true;
  }
  auto local_args = args;
  if (!local_args.empty() && tools::wallet2::parse_short_payment_id(local_args.back(), payment_id))
  {
    local_args.pop_back();
  }
  else
  {
    payment_id = crypto::rand<crypto::hash8>();
  }
  if (local_args.size() > 1)
  {
    fail_msg_writer() << tr("failed to parse payment id");
    return true;
  }

  uint32_t index_minor = 0;
  if (local_args.size() == 1)
  {
    if (!epee::string_tools::get_xtype_from_string(index_minor, local_args[0]))
    {
      fail_msg_writer() << tr("failed to parse index: ") << local_args[0];
      return true;
    }
  }

  if (index_minor >= m_wallet->get_num_subaddresses(m_current_subaddress_account))
  {
    fail_msg_writer() << tr("specify an index between 0 and ") << (m_wallet->get_num_subaddresses(m_current_subaddress_account) - 1) << tr(", or use \"address new\"");
    return true;
  }

  success_msg_writer() << m_wallet->get_integrated_subaddress_as_str({ m_current_subaddress_account, index_minor }, payment_id);
  success_msg_writer() << tr("Integrated payment ID: ") << epee::string_tools::pod_to_hex(payment_id);
  success_msg_writer() << tr("Address index: ") << index_minor;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::parse_address(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if (args.size() != 1)
  {
    success_msg_writer() << tr("usage: parse_address <addr>");
    return true;
  }

  cryptonote::address_parse_info info;
  if (get_account_address_from_str(info, m_wallet->testnet(), args.back()))
  {
    bool testnet = m_wallet->testnet();
    success_msg_writer() << tr("Address string: ") << cryptonote::get_account_address_as_str(testnet, info.is_subaddress, info.address);
    success_msg_writer() << tr("Type: ") << (info.is_subaddress ? tr("Subaddress") : tr("Standard address"));
    if (info.has_payment_id)
    {
      success_msg_writer() << tr("Integrated payment ID: ") << epee::string_tools::pod_to_hex(info.payment_id);
    }
  }
  else
  {
    fail_msg_writer() << tr("failed to parse given string");
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_tx_note(const std::vector<std::string> &args)
{
  if (args.size() == 0)
  {
    fail_msg_writer() << tr("usage: set_tx_note [txid] free text note");
    return true;
  }

  cryptonote::blobdata txid_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(args.front(), txid_data))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return false;
  }
  crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

  std::string note = "";
  for (size_t n = 1; n < args.size(); ++n)
  {
    if (n > 1)
      note += " ";
    note += args[n];
  }
  m_wallet->set_tx_note(txid, note);

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::get_tx_note(const std::vector<std::string> &args)
{
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: get_tx_note [txid]");
    return true;
  }

  cryptonote::blobdata txid_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(args.front(), txid_data))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return false;
  }
  crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

  std::string note = m_wallet->get_tx_note(txid);
  if (note.empty())
    success_msg_writer() << "no note found";
  else
    success_msg_writer() << "note found: " << note;

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::status(const std::vector<std::string> &args)
{
  uint64_t local_height = m_wallet->get_blockchain_current_height();
  uint32_t version = 0;
  if (!m_wallet->check_connection(&version))
  {
    success_msg_writer() << "Refreshed " << local_height << "/?, no daemon connected";
    return true;
  }

  std::string err;
  uint64_t bc_height = get_daemon_blockchain_height(err);
  if (err.empty())
  {
    bool synced = local_height == bc_height;
    success_msg_writer() << "Refreshed " << local_height << "/" << bc_height << ", " << (synced ? "synced" : "syncing")
        << ", daemon RPC v" << get_version_string(version);
  }
  else
  {
    fail_msg_writer() << "Refreshed " << local_height << "/?, daemon connection error";
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sign(const std::vector<std::string> &args)
{
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: sign <filename>");
    return true;
  }
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot sign");
    return true;
  }
  if (!get_and_verify_password()) { return true; }

  std::string filename = args[0];
  std::string data;
  bool r = epee::file_io_utils::load_file_to_string(filename, data);
  if (!r)
  {
    fail_msg_writer() << tr("failed to read file ") << filename;
    return true;
  }
  std::string signature = m_wallet->sign(data);
  success_msg_writer() << signature;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::verify(const std::vector<std::string> &args)
{
  if (args.size() != 3)
  {
    fail_msg_writer() << tr("usage: verify <filename> <address> <signature>");
    return true;
  }
  std::string filename = args[0];
  std::string address_string = args[1];
  std::string signature= args[2];

  std::string data;
  bool r = epee::file_io_utils::load_file_to_string(filename, data);
  if (!r)
  {
    fail_msg_writer() << tr("failed to read file ") << filename;
    return true;
  }

  cryptonote::address_parse_info info;
  if (!cryptonote::get_account_address_from_str_or_url(info, m_wallet->testnet(), address_string))
  {
    fail_msg_writer() << tr("failed to parse address");
    return true;
  }

  r = m_wallet->verify(data, info.address, signature);
  if (!r)
  {
    fail_msg_writer() << tr("Bad signature from ") << address_string;
  }
  else
  {
    success_msg_writer() << tr("Good signature from ") << address_string;
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::export_key_images(const std::vector<std::string> &args)
{
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: export_key_images <filename>");
    return true;
  }
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot export key images");
    return true;
  }
  if (!get_and_verify_password()) { return true; }

  std::string filename = args[0];

  try
  {
    std::vector<std::pair<crypto::key_image, crypto::signature>> ski = m_wallet->export_key_images();
    std::string magic(KEY_IMAGE_EXPORT_FILE_MAGIC, strlen(KEY_IMAGE_EXPORT_FILE_MAGIC));
    const cryptonote::account_public_address &keys = m_wallet->get_account().get_keys().m_account_address;

    std::string data;
    data += std::string((const char *)&keys.m_spend_public_key, sizeof(crypto::public_key));
    data += std::string((const char *)&keys.m_view_public_key, sizeof(crypto::public_key));
    for (const auto &i: ski)
    {
      data += std::string((const char *)&i.first, sizeof(crypto::key_image));
      data += std::string((const char *)&i.second, sizeof(crypto::signature));
    }

    // encrypt data, keep magic plaintext
    std::string ciphertext = m_wallet->encrypt_with_view_secret_key(data);
    bool r = epee::file_io_utils::save_string_to_file(filename, magic + ciphertext);
    if (!r)
    {
      fail_msg_writer() << tr("failed to save file ") << filename;
      return true;
    }
  }
  catch (std::exception &e)
  {
    LOG_ERROR("Error exporting key images: " << e.what());
    fail_msg_writer() << "Error exporting key images: " << e.what();
    return true;
  }

  success_msg_writer() << tr("Signed key images exported to ") << filename;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::import_key_images(const std::vector<std::string> &args)
{
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: import_key_images <filename>");
    return true;
  }
  std::string filename = args[0];

  std::string data;
  bool r = epee::file_io_utils::load_file_to_string(filename, data);
  if (!r)
  {
    fail_msg_writer() << tr("failed to read file ") << filename;
    return true;
  }
  const size_t magiclen = strlen(KEY_IMAGE_EXPORT_FILE_MAGIC);
  if (data.size() < magiclen || memcmp(data.data(), KEY_IMAGE_EXPORT_FILE_MAGIC, magiclen))
  {
    fail_msg_writer() << "Bad key image export file magic in " << filename;
    return true;
  }

  try
  {
    data = m_wallet->decrypt_with_view_secret_key(std::string(data, magiclen));
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << "Failed to decrypt " << filename << ": " << e.what();
    return true;
  }

  const size_t headerlen = 2 * sizeof(crypto::public_key);
  if (data.size() < headerlen)
  {
    fail_msg_writer() << "Bad data size from file " << filename;
    return true;
  }
  const crypto::public_key &public_spend_key = *(const crypto::public_key*)&data[0];
  const crypto::public_key &public_view_key = *(const crypto::public_key*)&data[sizeof(crypto::public_key)];
  const cryptonote::account_public_address &keys = m_wallet->get_account().get_keys().m_account_address;
  if (public_spend_key != keys.m_spend_public_key || public_view_key != keys.m_view_public_key)
  {
    fail_msg_writer() << "Key images from " << filename << " are for a different account";
    return true;
  }

  const size_t record_size = sizeof(crypto::key_image) + sizeof(crypto::signature);
  if ((data.size() - headerlen) % record_size)
  {
    fail_msg_writer() << "Bad data size from file " << filename;
    return true;
  }
  size_t nki = (data.size() - headerlen) / record_size;

  std::vector<std::pair<crypto::key_image, crypto::signature>> ski;
  ski.reserve(nki);
  for (size_t n = 0; n < nki; ++n)
  {
    crypto::key_image key_image = *reinterpret_cast<const crypto::key_image*>(&data[headerlen + n * record_size]);
    crypto::signature signature = *reinterpret_cast<const crypto::signature*>(&data[headerlen + n * record_size + sizeof(crypto::key_image)]);

    ski.push_back(std::make_pair(key_image, signature));
  }

  try
  {
    uint64_t spent = 0, unspent = 0;
    uint64_t height = m_wallet->import_key_images(ski, spent, unspent);
    success_msg_writer() << "Signed key images imported to height " << height << ", "
        << print_money(spent) << " spent, " << print_money(unspent) << " unspent";
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << "Failed to import key images: " << e.what();
    return true;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::export_outputs(const std::vector<std::string> &args)
{
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: export_outputs <filename>");
    return true;
  }
  if (!get_and_verify_password()) { return true; }

  std::string filename = args[0];

  try
  {
    std::vector<tools::wallet2::transfer_details> outs = m_wallet->export_outputs();

    std::stringstream oss;
    boost::archive::portable_binary_oarchive ar(oss);
    ar << outs;

    std::string magic(OUTPUT_EXPORT_FILE_MAGIC, strlen(OUTPUT_EXPORT_FILE_MAGIC));
    const cryptonote::account_public_address &keys = m_wallet->get_account().get_keys().m_account_address;
    std::string header;
    header += std::string((const char *)&keys.m_spend_public_key, sizeof(crypto::public_key));
    header += std::string((const char *)&keys.m_view_public_key, sizeof(crypto::public_key));
    std::string ciphertext = m_wallet->encrypt_with_view_secret_key(header + oss.str());
    bool r = epee::file_io_utils::save_string_to_file(filename, magic + ciphertext);
    if (!r)
    {
      fail_msg_writer() << tr("failed to save file ") << filename;
      return true;
    }
  }
  catch (const std::exception &e)
  {
    LOG_ERROR("Error exporting outputs: " << e.what());
    fail_msg_writer() << "Error exporting outputs: " << e.what();
    return true;
  }

  success_msg_writer() << tr("Outputs exported to ") << filename;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::import_outputs(const std::vector<std::string> &args)
{
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: import_outputs <filename>");
    return true;
  }
  std::string filename = args[0];

  std::string data;
  bool r = epee::file_io_utils::load_file_to_string(filename, data);
  if (!r)
  {
    fail_msg_writer() << tr("failed to read file ") << filename;
    return true;
  }
  const size_t magiclen = strlen(OUTPUT_EXPORT_FILE_MAGIC);
  if (data.size() < magiclen || memcmp(data.data(), OUTPUT_EXPORT_FILE_MAGIC, magiclen))
  {
    fail_msg_writer() << "Bad output export file magic in " << filename;
    return true;
  }

  try
  {
    data = m_wallet->decrypt_with_view_secret_key(std::string(data, magiclen));
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << "Failed to decrypt " << filename << ": " << e.what();
    return true;
  }

  const size_t headerlen = 2 * sizeof(crypto::public_key);
  if (data.size() < headerlen)
  {
    fail_msg_writer() << "Bad data size from file " << filename;
    return true;
  }
  const crypto::public_key &public_spend_key = *(const crypto::public_key*)&data[0];
  const crypto::public_key &public_view_key = *(const crypto::public_key*)&data[sizeof(crypto::public_key)];
  const cryptonote::account_public_address &keys = m_wallet->get_account().get_keys().m_account_address;
  if (public_spend_key != keys.m_spend_public_key || public_view_key != keys.m_view_public_key)
  {
    fail_msg_writer() << "Outputs from " << filename << " are for a different account";
    return true;
  }

  try
  {
    std::string body(data, headerlen);
    std::stringstream iss;
    iss << body;
    std::vector<tools::wallet2::transfer_details> outputs;
    try
    {
      boost::archive::portable_binary_iarchive ar(iss);
      ar >> outputs;
    }
    catch (...)
    {
      iss.str("");
      iss << body;
      boost::archive::binary_iarchive ar(iss);
      ar >> outputs;
    }
    size_t n_outputs = m_wallet->import_outputs(outputs);
    success_msg_writer() << boost::lexical_cast<std::string>(n_outputs) << " outputs imported";
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << "Failed to import outputs: " << e.what();
    return true;
  }


  /*try
  {
    std::string body(data, headerlen);
    std::stringstream iss;
    iss << body;
    boost::archive::binary_iarchive ar(iss);
    std::vector<tools::wallet2::transfer_details> outputs;
    ar >> outputs;

    size_t n_outputs = m_wallet->import_outputs(outputs);
    success_msg_writer() << boost::lexical_cast<std::string>(n_outputs) << " outputs imported";
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << "Failed to import outputs: " << e.what();
    return true;
  }*/

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::process_command(const std::vector<std::string> &args)
{
  return m_cmd_binder.process_command_vec(args);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::interrupt()
{
  if (m_in_manual_refresh.load(std::memory_order_relaxed))
  {
    m_wallet->stop();
  }
  else
  {
    stop();
  }
}
//----------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  po::options_description desc_params(wallet_args::tr("Wallet options"));
  tools::wallet2::init_options(desc_params);
  command_line::add_arg(desc_params, arg_wallet_file);
  command_line::add_arg(desc_params, arg_generate_new_wallet);
  command_line::add_arg(desc_params, arg_generate_from_view_key);
  command_line::add_arg(desc_params, arg_generate_from_keys);
  command_line::add_arg(desc_params, arg_generate_from_json);
  command_line::add_arg(desc_params, arg_command);

  command_line::add_arg(desc_params, arg_restore_deterministic_wallet );
  command_line::add_arg(desc_params, arg_non_deterministic );
  command_line::add_arg(desc_params, arg_electrum_seed );
  command_line::add_arg(desc_params, arg_trusted_daemon);
  command_line::add_arg(desc_params, arg_allow_mismatched_daemon_version);
  command_line::add_arg(desc_params, arg_restore_height);

  po::positional_options_description positional_options;
  positional_options.add(arg_command.name, -1);

  const auto vm = wallet_args::main(
   argc, argv,
   "sumo-wallet-cli [--wallet-file=<file>|--generate-new-wallet=<file>] [<COMMAND>]",
    desc_params,
    positional_options
  );

  if (!vm)
  {
    return 1;
  }

  cryptonote::simple_wallet w;
  w.init(*vm);

  std::vector<std::string> command = command_line::get_arg(*vm, arg_command);
  if (!command.empty())
  {
    w.process_command(command);
    w.stop();
    w.deinit();
  }
  else
  {
    tools::signal_handler::install([&w](int type) {
#ifdef WIN32
      if (type == CTRL_C_EVENT)
#else
      if (type == SIGINT)
#endif
      {
        // if we're pressing ^C when refreshing, just stop refreshing
        w.interrupt();
      }
      else
      {
        w.stop();
      }
    });
    w.run();

    w.deinit();
  }
  return 0;
  //CATCH_ENTRY_L0("main", 1);
}
