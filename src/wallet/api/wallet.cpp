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


#include "wallet.h"
#include "pending_transaction.h"
#include "transaction_history.h"
#include "address_book.h"
#include "subaddress.h"
#include "subaddress_account.h"
#include "common_defines.h"

#include "mnemonics/electrum-words.h"
#include <boost/format.hpp>
#include <sstream>
#include <unordered_map>

using namespace std;
using namespace cryptonote;

namespace Monero {

namespace {
    // copy-pasted from simplewallet
    //static const size_t DEFAULT_MIXIN = 4;
    static const int    DEFAULT_REFRESH_INTERVAL_MILLIS = 1000 * 10;
    // limit maximum refresh interval as one minute
    static const int    MAX_REFRESH_INTERVAL_MILLIS = 1000 * 60 * 1;
}

struct Wallet2CallbackImpl : public tools::i_wallet2_callback
{

    Wallet2CallbackImpl(WalletImpl * wallet)
     : m_listener(nullptr)
     , m_wallet(wallet)
    {

    }

    ~Wallet2CallbackImpl()
    {

    }

    void setListener(WalletListener * listener)
    {
        m_listener = listener;
    }

    WalletListener * getListener() const
    {
        return m_listener;
    }

    virtual void on_new_block(uint64_t height, const cryptonote::block& block)
    {
        LOG_PRINT_L3(__FUNCTION__ << ": new block. height: " << height);

        if (m_listener) {
            m_listener->newBlock(height);
            //  m_listener->updated();
        }
    }

    virtual void on_money_received(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t amount, const cryptonote::subaddress_index& subaddr_index)
    {

        std::string tx_hash =  epee::string_tools::pod_to_hex(get_transaction_hash(tx));

        LOG_PRINT_L3(__FUNCTION__ << ": money received. height:  " << height
                     << ", tx: " << tx_hash
                     << ", amount: " << print_money(amount)
                     << ", idx: " << subaddr_index);
        // do not signal on received tx if wallet is not syncronized completely
        if (m_listener && m_wallet->synchronized()) {
            m_listener->moneyReceived(tx_hash, amount);
            m_listener->updated();
        }
    }

    virtual void on_money_spent(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& in_tx, uint64_t amount,
      const cryptonote::transaction& spend_tx, const cryptonote::subaddress_index& subaddr_index)
    {
        // TODO;
        std::string tx_hash = epee::string_tools::pod_to_hex(get_transaction_hash(spend_tx));
        LOG_PRINT_L3(__FUNCTION__ << ": money spent. height:  " << height
                     << ", tx: " << tx_hash
                     << ", amount: " << print_money(amount)
                     << ", idx: " << subaddr_index);
        // do not signal on sent tx if wallet is not syncronized completely
        if (m_listener && m_wallet->synchronized()) {
            m_listener->moneySpent(tx_hash, amount);
            m_listener->updated();
        }
    }

    virtual void on_skip_transaction(uint64_t height, const cryptonote::transaction& tx)
    {
        // TODO;
    }

    WalletListener * m_listener;
    WalletImpl     * m_wallet;
};

Wallet::~Wallet() {}

WalletListener::~WalletListener() {}


string Wallet::displayAmount(uint64_t amount)
{
    return cryptonote::print_money(amount);
}

uint64_t Wallet::amountFromString(const string &amount)
{
    uint64_t result = 0;
    cryptonote::parse_amount(result, amount);
    return result;
}

uint64_t Wallet::amountFromDouble(double amount)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(CRYPTONOTE_DISPLAY_DECIMAL_POINT) << amount;
    return amountFromString(ss.str());
}

std::string Wallet::genPaymentId()
{
    crypto::hash8 payment_id = crypto::rand<crypto::hash8>();
    return epee::string_tools::pod_to_hex(payment_id);

}

bool Wallet::paymentIdValid(const string &paiment_id)
{
    crypto::hash8 pid8;
    if (tools::wallet2::parse_short_payment_id(paiment_id, pid8))
        return true;
    crypto::hash pid;
    if (tools::wallet2::parse_long_payment_id(paiment_id, pid))
        return true;
    return false;
}

bool Wallet::addressValid(const std::string &str, bool testnet)
{
  cryptonote::address_parse_info info;
  return get_account_address_from_str(info, testnet, str);
}

std::string Wallet::paymentIdFromAddress(const std::string &str, bool testnet)
{
  cryptonote::address_parse_info info;
  if (!get_account_address_from_str(info, testnet, str))
    return "";
  if (!info.has_payment_id)
    return "";
  return epee::string_tools::pod_to_hex(info.payment_id);
}

uint64_t Wallet::maximumAllowedAmount()
{
    return std::numeric_limits<uint64_t>::max();
}


///////////////////////// WalletImpl implementation ////////////////////////
WalletImpl::WalletImpl(bool testnet)
    :m_wallet(nullptr)
    , m_status(Wallet::Status_Ok)
    , m_trustedDaemon(false)
    , m_wallet2Callback(nullptr)
    , m_recoveringFromSeed(false)
    , m_synchronized(false)
    , m_rebuildWalletCache(false)
{
    m_wallet = new tools::wallet2(testnet);
    m_history = new TransactionHistoryImpl(this);
    m_wallet2Callback = new Wallet2CallbackImpl(this);
    m_wallet->callback(m_wallet2Callback);
    m_refreshThreadDone = false;
    m_refreshEnabled = false;
    m_addressBook = new AddressBookImpl(this);
    m_subaddress = new SubaddressImpl(this);
    m_subaddressAccount = new SubaddressAccountImpl(this);


    m_refreshIntervalMillis = DEFAULT_REFRESH_INTERVAL_MILLIS;

    m_refreshThread = boost::thread([this] () {
        this->refreshThreadFunc();
    });

}

WalletImpl::~WalletImpl()
{
    // Pause refresh thread - prevents refresh from starting again
    pauseRefresh();
    // Close wallet - stores cache and stops ongoing refresh operation
    close(false);
    // Stop refresh thread
    stopRefresh();
    delete m_history;
    delete m_addressBook;
    delete m_wallet;
    delete m_wallet2Callback;
    delete m_subaddress;
    delete m_subaddressAccount;
}

bool WalletImpl::create(const std::string &path, const std::string &password, const std::string &language)
{

    clearStatus();
    m_recoveringFromSeed = false;
    bool keys_file_exists;
    bool wallet_file_exists;
    tools::wallet2::wallet_exists(path, keys_file_exists, wallet_file_exists);
    LOG_PRINT_L3("wallet_path: " << path << "");
    LOG_PRINT_L3("keys_file_exists: " << std::boolalpha << keys_file_exists << std::noboolalpha
                 << "  wallet_file_exists: " << std::boolalpha << wallet_file_exists << std::noboolalpha);


    // add logic to error out if new wallet requested but named wallet file exists
    if (keys_file_exists || wallet_file_exists) {
        m_errorString = "attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.";
        LOG_ERROR(m_errorString);
        m_status = Status_Critical;
        return false;
    }
    // TODO: validate language
    m_wallet->set_seed_language(language);
    crypto::secret_key recovery_val, secret_key;
    try {
        recovery_val = m_wallet->generate(path, password, secret_key, false, false);
        m_password = password;
        m_status = Status_Ok;
    } catch (const std::exception &e) {
        LOG_ERROR("Error creating wallet: " << e.what());
        m_status = Status_Critical;
        m_errorString = e.what();
        return false;
    }

    return true;
}

bool WalletImpl::open(const std::string &path, const std::string &password)
{
    clearStatus();
    m_recoveringFromSeed = false;
    try {
        // TODO: handle "deprecated"
        // Check if wallet cache exists
        bool keys_file_exists;
        bool wallet_file_exists;
        tools::wallet2::wallet_exists(path, keys_file_exists, wallet_file_exists);
        if(!wallet_file_exists){
            // Rebuilding wallet cache, using refresh height from .keys file
            m_rebuildWalletCache = true;
        }
        m_wallet->load(path, password);

        m_password = password;
    } catch (const std::exception &e) {
        LOG_ERROR("Error opening wallet: " << e.what());
        m_status = Status_Critical;
        m_errorString = e.what();
    }
    return m_status == Status_Ok;
}
bool WalletImpl::recoverFromKeys(const std::string &path,
                                const std::string &language,
                                const std::string &address_string,
                                const std::string &viewkey_string,
                                const std::string &spendkey_string)
{
    cryptonote::address_parse_info info;
    if(!get_account_address_from_str(info, m_wallet->testnet(), address_string))
    {
        m_errorString = tr("failed to parse address");
        m_status = Status_Error;
        return false;
    }

    // parse optional spend key
    crypto::secret_key spendkey;
    bool has_spendkey = false;
    if (!spendkey_string.empty()) {
        cryptonote::blobdata spendkey_data;
        if(!epee::string_tools::parse_hexstr_to_binbuff(spendkey_string, spendkey_data) || spendkey_data.size() != sizeof(crypto::secret_key))
        {
            m_errorString = tr("failed to parse secret spend key");
            m_status = Status_Error;
            return false;
        }
        has_spendkey = true;
        spendkey = *reinterpret_cast<const crypto::secret_key*>(spendkey_data.data());
    }

    // parse view secret key
    if (viewkey_string.empty()) {
        m_errorString = tr("No view key supplied, cancelled");
        m_status = Status_Error;
        return false;
    }
    cryptonote::blobdata viewkey_data;
    if(!epee::string_tools::parse_hexstr_to_binbuff(viewkey_string, viewkey_data) || viewkey_data.size() != sizeof(crypto::secret_key))
    {
        m_errorString = tr("failed to parse secret view key");
        m_status = Status_Error;
        return false;
    }
    crypto::secret_key viewkey = *reinterpret_cast<const crypto::secret_key*>(viewkey_data.data());

    // check the spend and view keys match the given address
    crypto::public_key pkey;
    if(has_spendkey) {
        if (!crypto::secret_key_to_public_key(spendkey, pkey)) {
            m_errorString = tr("failed to verify secret spend key");
            m_status = Status_Error;
            return false;
        }
        if (info.address.m_spend_public_key != pkey) {
            m_errorString = tr("spend key does not match address");
            m_status = Status_Error;
            return false;
        }
    }
    if (!crypto::secret_key_to_public_key(viewkey, pkey)) {
        m_errorString = tr("failed to verify secret view key");
        m_status = Status_Error;
        return false;
    }
    if (info.address.m_view_public_key != pkey) {
        m_errorString = tr("view key does not match address");
        m_status = Status_Error;
        return false;
    }

    try
    {
        if (has_spendkey) {
            m_wallet->generate(path, "", info.address, spendkey, viewkey);
            setSeedLanguage(language);
            LOG_PRINT_L1("Generated new wallet from keys with seed language: " + language);
        }
        else {
            m_wallet->generate(path, "", info.address, viewkey);
            LOG_PRINT_L1("Generated new view only wallet from keys");
        }

    }
    catch (const std::exception& e) {
        m_errorString = string(tr("failed to generate new wallet: ")) + e.what();
        m_status = Status_Error;
        return false;
    }
    return true;
}


bool WalletImpl::recover(const std::string &path, const std::string &seed)
{
    clearStatus();
    m_errorString.clear();
    if (seed.empty()) {
        m_errorString = "Electrum seed is empty";
        LOG_ERROR(m_errorString);
        m_status = Status_Error;
        return false;
    }

    m_recoveringFromSeed = true;
    crypto::secret_key recovery_key;
    std::string old_language;
    if (!crypto::ElectrumWords::words_to_bytes(seed, recovery_key, old_language)) {
        m_errorString = "Electrum-style word list failed verification";
        m_status = Status_Error;
        return false;
    }

    try {
        m_wallet->set_seed_language(old_language);
        m_wallet->generate(path, "", recovery_key, true, false);
        // TODO: wallet->init(daemon_address);

    } catch (const std::exception &e) {
        m_status = Status_Critical;
        m_errorString = e.what();
    }
    return m_status == Status_Ok;
}

bool WalletImpl::close(bool store)
{

    bool result = false;
    LOG_PRINT_L3("closing wallet...");
    try {
        if (store) {
            // Do not store wallet with invalid status
            // Status Critical refers to errors on opening or creating wallets.
            if (status() != Status_Critical)
                m_wallet->store();
            else
                LOG_ERROR("Status_Critical - not storing wallet");
            LOG_PRINT_L3("wallet::store done");
        }
        LOG_PRINT_L3("Calling wallet::stop...");
        m_wallet->stop();
        LOG_PRINT_L3("wallet::stop done");
        result = true;
        clearStatus();
    } catch (const std::exception &e) {
        m_status = Status_Critical;
        m_errorString = e.what();
        LOG_ERROR("Error closing wallet: " << e.what());
    }
    return result;
}

std::string WalletImpl::seed() const
{
    std::string seed;
    if (m_wallet)
        m_wallet->get_seed(seed);
    return seed;
}

std::string WalletImpl::getSeedLanguage() const
{
    return m_wallet->get_seed_language();
}

void WalletImpl::setSeedLanguage(const std::string &arg)
{
    m_wallet->set_seed_language(arg);
}

int WalletImpl::status() const
{
    return m_status;
}

std::string WalletImpl::errorString() const
{
    return m_errorString;
}

bool WalletImpl::setPassword(const std::string &password)
{
    clearStatus();
    try {
        m_wallet->rewrite(m_wallet->get_wallet_file(), password);
        m_password = password;
    } catch (const std::exception &e) {
        m_status = Status_Error;
        m_errorString = e.what();
    }
    return m_status == Status_Ok;
}

std::string WalletImpl::address(uint32_t accountIndex, uint32_t addressIndex) const
{
  return m_wallet->get_subaddress_as_str({ accountIndex, addressIndex });
}

std::string WalletImpl::integratedAddress(uint32_t accountIndex, uint32_t addressIndex, const std::string &payment_id) const
{
  crypto::hash8 pid;
  if (!tools::wallet2::parse_short_payment_id(payment_id, pid)) {
    return "";
  }
  return m_wallet->get_integrated_subaddress_as_str({ accountIndex, addressIndex }, pid);
}

std::string WalletImpl::secretViewKey() const
{
    return epee::string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_view_secret_key);
}

std::string WalletImpl::publicViewKey() const
{
    return epee::string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_account_address.m_view_public_key);
}

std::string WalletImpl::secretSpendKey() const
{
    return epee::string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_spend_secret_key);
}

std::string WalletImpl::publicSpendKey() const
{
    return epee::string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_account_address.m_spend_public_key);
}

std::string WalletImpl::path() const
{
    return m_wallet->path();
}

bool WalletImpl::store(const std::string &path)
{
    clearStatus();
    try {
        if (path.empty()) {
            m_wallet->store();
        } else {
            m_wallet->store_to(path, m_password);
        }
    } catch (const std::exception &e) {
        LOG_ERROR("Error storing wallet: " << e.what());
        m_status = Status_Error;
        m_errorString = e.what();
    }

    return m_status == Status_Ok;
}

string WalletImpl::filename() const
{
    return m_wallet->get_wallet_file();
}

string WalletImpl::keysFilename() const
{
    return m_wallet->get_keys_file();
}

void WalletImpl::init(const std::string &daemon_address, uint64_t upper_transaction_size_limit, bool enable_ssl, const char* cacerts_path)
{
    clearStatus();
    doInit(daemon_address, upper_transaction_size_limit, enable_ssl, cacerts_path);
}

void WalletImpl::initAsync(const string &daemon_address, uint64_t upper_transaction_size_limit, bool enable_ssl, const char* cacerts_path)
{
    clearStatus();
    doInit(daemon_address, upper_transaction_size_limit, enable_ssl, cacerts_path);
    startRefresh();
}

void WalletImpl::setRefreshFromBlockHeight(uint64_t refresh_from_block_height)
{
    m_wallet->set_refresh_from_block_height(refresh_from_block_height);
}

void WalletImpl::setRecoveringFromSeed(bool recoveringFromSeed)
{
    m_recoveringFromSeed = recoveringFromSeed;
}

uint64_t WalletImpl::balance(uint32_t accountIndex) const
{
  return m_wallet->balance(accountIndex);
}

uint64_t WalletImpl::unlockedBalance(uint32_t accountIndex) const
{
  return m_wallet->unlocked_balance(accountIndex);
}

uint64_t WalletImpl::blockChainHeight() const
{
    return m_wallet->get_blockchain_current_height();
}
uint64_t WalletImpl::approximateBlockChainHeight() const
{
    return m_wallet->get_approximate_blockchain_height();
}
uint64_t WalletImpl::daemonBlockChainHeight() const
{
    std::string err;
    uint64_t result = m_wallet->get_daemon_blockchain_height(err);
    if (!err.empty()) {
        LOG_ERROR(__FUNCTION__ << ": " << err);
        result = 0;
        m_errorString = err;
        m_status = Status_Error;

    } else {
        m_status = Status_Ok;
        m_errorString = "";
    }
    return result;
}

uint64_t WalletImpl::daemonBlockChainTargetHeight() const
{
    std::string err;
    uint64_t result = m_wallet->get_daemon_blockchain_target_height(err);
    if (!err.empty()) {
        LOG_ERROR(__FUNCTION__ << ": " << err);
        result = 0;
        m_errorString = err;
        m_status = Status_Error;

    } else {
        m_status = Status_Ok;
        m_errorString = "";
    }
    return result;
}

bool WalletImpl::synchronized() const
{
    return m_synchronized;
}

bool WalletImpl::refresh()
{
    clearStatus();
    doRefresh();
    return m_status == Status_Ok;
}

void WalletImpl::refreshAsync()
{
    LOG_PRINT_L3(__FUNCTION__ << ": Refreshing asynchronously..");
    clearStatus();
    m_refreshCV.notify_one();
}

void WalletImpl::setAutoRefreshInterval(int millis)
{
    if (millis > MAX_REFRESH_INTERVAL_MILLIS) {
        LOG_ERROR(__FUNCTION__<< ": invalid refresh interval " << millis
                  << " ms, maximum allowed is " << MAX_REFRESH_INTERVAL_MILLIS << " ms");
        m_refreshIntervalMillis = MAX_REFRESH_INTERVAL_MILLIS;
    } else {
        m_refreshIntervalMillis = millis;
    }
}

int WalletImpl::autoRefreshInterval() const
{
    return m_refreshIntervalMillis;
}

void WalletImpl::addSubaddressAccount(const std::string& label)
{
  m_wallet->add_subaddress_account(label);
}
size_t WalletImpl::numSubaddressAccounts() const
{
  return m_wallet->get_num_subaddress_accounts();
}
size_t WalletImpl::numSubaddresses(uint32_t accountIndex) const
{
  return m_wallet->get_num_subaddresses(accountIndex);
}
void WalletImpl::addSubaddress(uint32_t accountIndex, const std::string& label)
{
  m_wallet->add_subaddress(accountIndex, label);
}
std::string WalletImpl::getSubaddressLabel(uint32_t accountIndex, uint32_t addressIndex) const
{
  try
  {
    return m_wallet->get_subaddress_label({ accountIndex, addressIndex });
  }
  catch (const std::exception &e)
  {
    LOG_ERROR("Error getting subaddress label: " << e.what());
    m_errorString = string(tr("Failed to get subaddress label: ")) + e.what();
    m_status = Status_Error;
    return "";
  }
}
void WalletImpl::setSubaddressLabel(uint32_t accountIndex, uint32_t addressIndex, const std::string &label)
{
  try
  {
    return m_wallet->set_subaddress_label({ accountIndex, addressIndex }, label);
  }
  catch (const std::exception &e)
  {
    LOG_ERROR("Error setting subaddress label: " << e.what());
    m_errorString = string(tr("Failed to set subaddress label: ")) + e.what();
    m_status = Status_Error;
  }
}


// TODO:
// 1 - properly handle payment id (add another menthod with explicit 'payment_id' param)
// 2 - check / design how "Transaction" can be single interface
// (instead of few different data structures within wallet2 implementation:
//    - pending_tx;
//    - transfer_details;
//    - payment_details;
//    - unconfirmed_transfer_details;
//    - confirmed_transfer_details)

PendingTransaction *WalletImpl::createTransaction(const string &dst_addr, const string &payment_id, optional<uint64_t> amount, uint32_t mixin_count,
    PendingTransaction::Priority priority, uint32_t subaddr_account, std::set<uint32_t> subaddr_indices)
{
  clearStatus();
  // Pause refresh thread while creating transaction
  pauseRefresh();

  PendingTransactionImpl * transaction = new PendingTransactionImpl(*this);
  transaction->m_status = Status_Ok;

  size_t fake_outs_count = mixin_count;

  std::vector<uint8_t> extra;
  bool payment_id_seen = false;
  if (!payment_id.empty()) {
    std::string payment_id_str = payment_id;

    crypto::hash hash_payment_id;
    bool r = tools::wallet2::parse_long_payment_id(payment_id_str, hash_payment_id);
    if(r) {
      std::string extra_nonce;
      set_payment_id_to_tx_extra_nonce(extra_nonce, hash_payment_id);
      r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
    } else {
      crypto::hash8 payment_id8;
      r = tools::wallet2::parse_short_payment_id(payment_id_str, payment_id8);
      if(r) {
        std::string extra_nonce;
        set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, payment_id8);
        r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
      }
    }

    if(!r) {
      LOG_ERROR("payment id has invalid format, expected 16 or 64 character hex string: " << payment_id_str);
      return nullptr;
    }
    payment_id_seen = true;
  }

  uint64_t locked_blocks = 0;

  vector<cryptonote::tx_destination_entry> dsts;
  cryptonote::address_parse_info info;
  cryptonote::tx_destination_entry de;
  if (!cryptonote::get_account_address_from_str_or_url(info, m_wallet->testnet(), dst_addr)) {
    return nullptr;
  }

  de.addr = info.address;
  de.amount = *amount;

  if (info.has_payment_id) {
    if (payment_id_seen) {
      LOG_ERROR("a single transaction cannot use more than one payment id: ");
      return nullptr;
    }

    std::string extra_nonce;
    set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, info.payment_id);
    bool r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
    if(!r) {
      LOG_ERROR("failed to set up payment id, though it was decoded correctly");
      return nullptr;
    }
    payment_id_seen = true;
  }

  dsts.push_back(de);

  try
  {
    // figure out what tx will be necessary
    std::vector<tools::wallet2::pending_tx> ptx_vector;
    uint64_t bc_height, unlock_block = 0;
    ptx_vector = m_wallet->create_transactions_2(dsts, fake_outs_count, 0 /* unlock_time */, priority, extra, subaddr_account, subaddr_indices, false /* m_trusted_daemon */, 1.0f /* tx_size_target_factor */);

    // if more than one tx necessary
    if (m_wallet->always_confirm_transfers() || ptx_vector.size() > 1) {
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

      for (size_t n = 0; n < ptx_vector.size(); ++n)
      {
        subaddr_indices.clear();
        for (uint32_t i : ptx_vector[n].construction_data.subaddr_indices)
          subaddr_indices.insert(i);
        if (subaddr_indices.size() > 1)
          LOG_ERROR("WARNING: Outputs of multiple addresses are being used together, which might potentially compromise your privacy.\n");
      }
    }

    transaction->m_pending_tx = ptx_vector;
    startRefresh();
    return transaction;

  }
  catch (const tools::error::daemon_busy&)
  {
    LOG_ERROR("daemon is busy. Please try again later.");
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    LOG_ERROR("no connection to daemon. Please make sure daemon is running.");
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("RPC error: " << e.to_string());
  }
  catch (const tools::error::get_random_outs_error&)
  {
    LOG_ERROR("failed to get random outputs to mix");
  }
  catch (const tools::error::not_enough_money& e)
  {
    LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, sent amount %s") %
        print_money(e.available()) %
        print_money(e.tx_amount()));
  }
  catch (const tools::error::tx_not_possible& e)
  {
    LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)") %
        print_money(e.available()) %
        print_money(e.tx_amount() + e.fee())  %
        print_money(e.tx_amount()) %
        print_money(e.fee()));
  }
  catch (const tools::error::not_enough_outs_to_mix& e)
  {
    LOG_ERROR(("not enough outputs for specified mixin_count") << " = " << e.mixin_count() << ":");
  }
  catch (const tools::error::tx_not_constructed&)
  {
    LOG_ERROR("transaction was not constructed");
  }
  catch (const tools::error::tx_rejected& e)
  {
    LOG_ERROR((boost::format(tr("transaction %s was rejected by daemon with status: ")) % get_transaction_hash(e.tx())) << e.status());
    std::string reason = e.reason();
    if (!reason.empty())
      LOG_ERROR("Reason: " << reason);
  }
  catch (const tools::error::tx_sum_overflow& e)
  {
    LOG_ERROR(e.what());
  }
  catch (const tools::error::zero_destination&)
  {
    LOG_ERROR("one of destinations is zero");
  }
  catch (const tools::error::tx_too_big& e)
  {
    LOG_ERROR("transaction too big: " << e.to_string());
  }
  catch (const tools::error::transfer_error& e)
  {
    LOG_ERROR("unknown transfer error: " << e.to_string());
  }
  catch (const tools::error::wallet_internal_error& e)
  {
    LOG_ERROR("internal error: " << e.to_string());
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
  }
  catch (...)
  {
    LOG_ERROR("unknown error");
  }

  transaction->m_status = Status_Error;
  startRefresh();
  return transaction;
}

PendingTransaction *WalletImpl::createSweepUnmixableTransaction()

{
    clearStatus();
    vector<cryptonote::tx_destination_entry> dsts;
    cryptonote::tx_destination_entry de;

    PendingTransactionImpl * transaction = new PendingTransactionImpl(*this);

    do {
        try {
            transaction->m_pending_tx = m_wallet->create_unmixable_sweep_transactions(m_trustedDaemon);

        } catch (const tools::error::daemon_busy&) {
            // TODO: make it translatable with "tr"?
            m_errorString = tr("daemon is busy. Please try again later.");
            m_status = Status_Error;
        } catch (const tools::error::no_connection_to_daemon&) {
            m_errorString = tr("no connection to daemon. Please make sure daemon is running.");
            m_status = Status_Error;
        } catch (const tools::error::wallet_rpc_error& e) {
            m_errorString = tr("RPC error: ") +  e.to_string();
            m_status = Status_Error;
        } catch (const tools::error::get_random_outs_error&) {
            m_errorString = tr("failed to get random outputs to mix");
            m_status = Status_Error;

        } catch (const tools::error::not_enough_money& e) {
            m_status = Status_Error;
            std::ostringstream writer;

            writer << boost::format(tr("not enough money to transfer, available only %s, sent amount %s")) %
                      print_money(e.available()) %
                      print_money(e.tx_amount());
            m_errorString = writer.str();

        } catch (const tools::error::tx_not_possible& e) {
            m_status = Status_Error;
            std::ostringstream writer;

            writer << boost::format(tr("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)")) %
                      print_money(e.available()) %
                      print_money(e.tx_amount() + e.fee())  %
                      print_money(e.tx_amount()) %
                      print_money(e.fee());
            m_errorString = writer.str();

        } catch (const tools::error::not_enough_outs_to_mix& e) {
            std::ostringstream writer;
            writer << tr("not enough outputs for specified mixin_count") << " = " << e.mixin_count() << ":";
            for (const std::pair<uint64_t, uint64_t> outs_for_amount : e.scanty_outs()) {
                writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.first) << ", " << tr("found outputs to mix") << " = " << outs_for_amount.second;
            }
            m_errorString = writer.str();
            m_status = Status_Error;
        } catch (const tools::error::tx_not_constructed&) {
            m_errorString = tr("transaction was not constructed");
            m_status = Status_Error;
        } catch (const tools::error::tx_rejected& e) {
            std::ostringstream writer;
            writer << (boost::format(tr("transaction %s was rejected by daemon with status: ")) % get_transaction_hash(e.tx())) <<  e.status();
            m_errorString = writer.str();
            m_status = Status_Error;
        } catch (const tools::error::tx_sum_overflow& e) {
            m_errorString = e.what();
            m_status = Status_Error;
        } catch (const tools::error::zero_destination&) {
            m_errorString =  tr("one of destinations is zero");
            m_status = Status_Error;
        } catch (const tools::error::tx_too_big& e) {
            m_errorString =  tr("failed to find a suitable way to split transactions");
            m_status = Status_Error;
        } catch (const tools::error::transfer_error& e) {
            m_errorString = string(tr("unknown transfer error: ")) + e.what();
            m_status = Status_Error;
        } catch (const tools::error::wallet_internal_error& e) {
            m_errorString =  string(tr("internal error: ")) + e.what();
            m_status = Status_Error;
        } catch (const std::exception& e) {
            m_errorString =  string(tr("unexpected error: ")) + e.what();
            m_status = Status_Error;
        } catch (...) {
            m_errorString = tr("unknown error");
            m_status = Status_Error;
        }
    } while (false);

    transaction->m_status = m_status;
    transaction->m_errorString = m_errorString;
    return transaction;
}

void WalletImpl::disposeTransaction(PendingTransaction *t)
{
    delete t;
}

TransactionHistory *WalletImpl::history()
{
  return m_history;
}

AddressBook *WalletImpl::addressBook()
{
  return m_addressBook;
}

Subaddress *WalletImpl::subaddress()
{
  return m_subaddress;
}

SubaddressAccount *WalletImpl::subaddressAccount()
{
  return m_subaddressAccount;
}

void WalletImpl::setListener(WalletListener *l)
{
    // TODO thread synchronization;
    m_wallet2Callback->setListener(l);
}

uint32_t WalletImpl::defaultMixin() const
{
    return m_wallet->default_mixin();
}

void WalletImpl::setDefaultMixin(uint32_t arg)
{
    m_wallet->default_mixin(arg);
}

bool WalletImpl::setUserNote(const std::string &txid, const std::string &note)
{
    cryptonote::blobdata txid_data;
    if(!epee::string_tools::parse_hexstr_to_binbuff(txid, txid_data))
      return false;
    const crypto::hash htxid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

    m_wallet->set_tx_note(htxid, note);
    return true;
}

std::string WalletImpl::getUserNote(const std::string &txid) const
{
    cryptonote::blobdata txid_data;
    if(!epee::string_tools::parse_hexstr_to_binbuff(txid, txid_data))
      return "";
    const crypto::hash htxid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

    return m_wallet->get_tx_note(htxid);
}

std::string WalletImpl::getTxKey(const std::string &txid) const
{
    cryptonote::blobdata txid_data;
    if(!epee::string_tools::parse_hexstr_to_binbuff(txid, txid_data))
    {
      return "";
    }
    const crypto::hash htxid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

    crypto::secret_key tx_key;
    if (m_wallet->get_tx_key(htxid, tx_key))
    {
      return epee::string_tools::pod_to_hex(tx_key);
    }
    else
    {
      return "";
    }
}

std::string WalletImpl::signMessage(const std::string &message)
{
  return m_wallet->sign(message);
}

bool WalletImpl::verifySignedMessage(const std::string &message, const std::string &address, const std::string &signature) const
{
  cryptonote::address_parse_info info;

  if (!cryptonote::get_account_address_from_str(info, m_wallet->testnet(), address))
    return false;

  return m_wallet->verify(message, info.address, signature);
}

bool WalletImpl::connectToDaemon()
{
    bool result = m_wallet->check_connection();
    m_status = result ? Status_Ok : Status_Error;
    if (!result) {
        m_errorString = "Error connecting to daemon at " + m_wallet->get_daemon_address();
    } else {
        // start refreshing here
    }
    return result;
}

Wallet::ConnectionStatus WalletImpl::connected() const
{
    uint32_t version = 0;
    bool is_connected = m_wallet->check_connection(&version);
    if (!is_connected)
        return Wallet::ConnectionStatus_Disconnected;
    if ((version >> 16) != CORE_RPC_VERSION_MAJOR)
        return Wallet::ConnectionStatus_WrongVersion;
    return Wallet::ConnectionStatus_Connected;
}

void WalletImpl::setTrustedDaemon(bool arg)
{
    m_trustedDaemon = arg;
}

bool WalletImpl::watchOnly() const
{
    return m_wallet->watch_only();
}

bool WalletImpl::trustedDaemon() const
{
    return m_trustedDaemon;
}

void WalletImpl::clearStatus()
{
    m_status = Status_Ok;
    m_errorString.clear();
}

void WalletImpl::refreshThreadFunc()
{
    LOG_PRINT_L3(__FUNCTION__ << ": starting refresh thread");

    while (true) {
        boost::mutex::scoped_lock lock(m_refreshMutex);
        if (m_refreshThreadDone) {
            break;
        }
        LOG_PRINT_L3(__FUNCTION__ << ": waiting for refresh...");
        // if auto refresh enabled, we wait for the "m_refreshIntervalSeconds" interval.
        // if not - we wait forever
        if (m_refreshIntervalMillis > 0) {
            boost::posix_time::milliseconds wait_for_ms(m_refreshIntervalMillis);
            m_refreshCV.timed_wait(lock, wait_for_ms);
        } else {
            m_refreshCV.wait(lock);
        }

        LOG_PRINT_L3(__FUNCTION__ << ": refresh lock acquired...");
        LOG_PRINT_L3(__FUNCTION__ << ": m_refreshEnabled: " << m_refreshEnabled);
        LOG_PRINT_L3(__FUNCTION__ << ": m_status: " << m_status);
        if (m_refreshEnabled /*&& m_status == Status_Ok*/) {
            LOG_PRINT_L3(__FUNCTION__ << ": refreshing...");
            doRefresh();
        }
    }
    LOG_PRINT_L3(__FUNCTION__ << ": refresh thread stopped");
}

void WalletImpl::doRefresh()
{
    // synchronizing async and sync refresh calls
    boost::lock_guard<boost::mutex> guarg(m_refreshMutex2);
    try {
        m_wallet->refresh();
        if (!m_synchronized) {
            m_synchronized = true;
        }
        // assuming if we have empty history, it wasn't initialized yet
        // for futher history changes client need to update history in
        // "on_money_received" and "on_money_sent" callbacks
        if (m_history->count() == 0) {
            m_history->refresh();
        }
    } catch (const std::exception &e) {
        m_status = Status_Error;
        m_errorString = e.what();
    }
    if (m_wallet2Callback->getListener()) {
        m_wallet2Callback->getListener()->refreshed();
    }
}


void WalletImpl::startRefresh()
{
    LOG_PRINT_L2(__FUNCTION__ << ": refresh started/resumed...");
    if (!m_refreshEnabled) {
        m_refreshEnabled = true;
        m_refreshCV.notify_one();
    }
}



void WalletImpl::stopRefresh()
{
    if (!m_refreshThreadDone) {
        m_refreshEnabled = false;
        m_refreshThreadDone = true;
        m_refreshCV.notify_one();
        m_refreshThread.join();
    }
}

void WalletImpl::pauseRefresh()
{
    LOG_PRINT_L2(__FUNCTION__ << ": refresh paused...");
    // TODO synchronize access
    if (!m_refreshThreadDone) {
        m_refreshEnabled = false;
    }
}


bool WalletImpl::isNewWallet() const
{
    // in case wallet created without daemon connection, closed and opened again,
    // it's the same case as if it created from scratch, i.e. we need "fast sync"
    // with the daemon (pull hashes instead of pull blocks).
    // If wallet cache is rebuilt, creation height stored in .keys is used.
    return !(blockChainHeight() > 1 || m_recoveringFromSeed || m_rebuildWalletCache);
}

void WalletImpl::doInit(const string &daemon_address, uint64_t upper_transaction_size_limit, bool enable_ssl, const char* cacerts_path)
{
    m_wallet->init(daemon_address, upper_transaction_size_limit, enable_ssl, cacerts_path);

    // in case new wallet, this will force fast-refresh (pulling hashes instead of blocks)
    if (isNewWallet()) {
        m_wallet->set_refresh_from_block_height(daemonBlockChainHeight());
    }

    if (Utils::isAddressLocal(daemon_address)) {
        this->setTrustedDaemon(true);
    }

}

} // namespace

namespace Bitmonero = Monero;
