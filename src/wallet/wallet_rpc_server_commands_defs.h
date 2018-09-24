// Copyright (c) 2014-2018, The Monero Project
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

#pragma once
#include "cryptonote_config.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/subaddress_index.h"
#include "crypto/hash.h"
#include "wallet_rpc_server_error_codes.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.rpc"

namespace tools
{
namespace wallet_rpc
{
#define WALLET_RPC_STATUS_OK      "OK"
#define WALLET_RPC_STATUS_BUSY    "BUSY"

  struct COMMAND_RPC_GET_BALANCE
  {
    struct request
    {
      uint32_t account_index;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(account_index)
      END_KV_SERIALIZE_MAP()
    };

    struct per_subaddress_info
    {
      uint32_t address_index;
      std::string address;
      uint64_t balance;
      uint64_t unlocked_balance;
      std::string label;
      uint64_t num_unspent_outputs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address_index)
        KV_SERIALIZE(address)
        KV_SERIALIZE(balance)
        KV_SERIALIZE(unlocked_balance)
        KV_SERIALIZE(label)
        KV_SERIALIZE(num_unspent_outputs)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t 	 balance;
      uint64_t 	 unlocked_balance;
      bool       multisig_import_needed;
      std::vector<per_subaddress_info> per_subaddress;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(balance)
        KV_SERIALIZE(unlocked_balance)
        KV_SERIALIZE(multisig_import_needed)
        KV_SERIALIZE(per_subaddress)
      END_KV_SERIALIZE_MAP()
    };
  };

    struct COMMAND_RPC_GET_ADDRESS
  {
    struct request
    {
      uint32_t account_index;
      std::vector<uint32_t> address_index;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(address_index)
      END_KV_SERIALIZE_MAP()
    };

    struct address_info
    {
      std::string address;
      std::string label;
      uint32_t address_index;
      bool used;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(label)
        KV_SERIALIZE(address_index)
        KV_SERIALIZE(used)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string address;                  // to remain compatible with older RPC format
      std::vector<address_info> addresses;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(addresses)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_CREATE_ADDRESS
  {
    struct request
    {
      uint32_t account_index;
      std::string label;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(label)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string   address;
      uint32_t      address_index;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(address_index)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_LABEL_ADDRESS
  {
    struct request
    {
      cryptonote::subaddress_index index;
      std::string label;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(index)
        KV_SERIALIZE(label)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_ACCOUNTS
  {
    struct request
    {
      std::string tag;      // all accounts if empty, otherwise those accounts with this tag

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tag)
      END_KV_SERIALIZE_MAP()
    };

    struct subaddress_account_info
    {
      uint32_t account_index;
      std::string base_address;
      uint64_t balance;
      uint64_t unlocked_balance;
      std::string label;
      std::string tag;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(base_address)
        KV_SERIALIZE(balance)
        KV_SERIALIZE(unlocked_balance)
        KV_SERIALIZE(label)
        KV_SERIALIZE(tag)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t total_balance;
      uint64_t total_unlocked_balance;
      std::vector<subaddress_account_info> subaddress_accounts;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(total_balance)
        KV_SERIALIZE(total_unlocked_balance)
        KV_SERIALIZE(subaddress_accounts)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_CREATE_ACCOUNT
  {
    struct request
    {
      std::string label;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(label)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint32_t account_index;
      std::string address;      // the 0-th address for convenience
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(address)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_LABEL_ACCOUNT
  {
    struct request
    {
      uint32_t account_index;
      std::string label;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(label)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_ACCOUNT_TAGS
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct account_tag_info
    {
      std::string tag;
      std::string label;
      std::vector<uint32_t> accounts;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tag);
        KV_SERIALIZE(label);
        KV_SERIALIZE(accounts);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<account_tag_info> account_tags;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(account_tags)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_TAG_ACCOUNTS
  {
    struct request
    {
      std::string tag;
      std::set<uint32_t> accounts;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tag)
        KV_SERIALIZE(accounts)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_UNTAG_ACCOUNTS
  {
    struct request
    {
      std::set<uint32_t> accounts;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(accounts)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SET_ACCOUNT_TAG_DESCRIPTION
  {
    struct request
    {
      std::string tag;
      std::string description;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tag)
        KV_SERIALIZE(description)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

    struct COMMAND_RPC_GET_HEIGHT
    {
      struct request
      {
        BEGIN_KV_SERIALIZE_MAP()
        END_KV_SERIALIZE_MAP()
      };

      struct response
      {
        uint64_t  height;
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(height)
        END_KV_SERIALIZE_MAP()
      };
    };

  struct transfer_destination
  {
    uint64_t amount;
    std::string address;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE(address)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_TRANSFER
  {
    struct request
    {
      std::list<transfer_destination> destinations;
      uint32_t account_index;
      std::set<uint32_t> subaddr_indices;
      uint32_t priority;
      uint64_t mixin;
      uint64_t ring_size;
      uint64_t unlock_time;
      std::string payment_id;
      bool get_tx_key;
      bool do_not_relay;
      bool get_tx_hex;
      bool get_tx_metadata;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(destinations)
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(subaddr_indices)
        KV_SERIALIZE(priority)
        KV_SERIALIZE_OPT(mixin, (uint64_t)0)
        KV_SERIALIZE_OPT(ring_size, (uint64_t)0)
        KV_SERIALIZE(unlock_time)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(get_tx_key)
        KV_SERIALIZE_OPT(do_not_relay, false)
        KV_SERIALIZE_OPT(get_tx_hex, false)
        KV_SERIALIZE_OPT(get_tx_metadata, false)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string tx_hash;
      std::string tx_key;
      std::list<std::string> amount_keys;
      uint64_t amount;
      uint64_t fee;
      std::string tx_blob;
      std::string tx_metadata;
      std::string multisig_txset;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
        KV_SERIALIZE(tx_key)
        KV_SERIALIZE(amount_keys)
        KV_SERIALIZE(amount)
        KV_SERIALIZE(fee)
        KV_SERIALIZE(tx_blob)
        KV_SERIALIZE(tx_metadata)
        KV_SERIALIZE(multisig_txset)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_TRANSFER_SPLIT
  {
    struct request
    {
      std::list<transfer_destination> destinations;
      uint32_t account_index;
      std::set<uint32_t> subaddr_indices;
      uint32_t priority;
      uint64_t mixin;
      uint64_t ring_size;
      uint64_t unlock_time;
      std::string payment_id;
      bool get_tx_keys;
      bool do_not_relay;
      bool get_tx_hex;
      bool get_tx_metadata;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(destinations)
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(subaddr_indices)
        KV_SERIALIZE(priority)
        KV_SERIALIZE_OPT(mixin, (uint64_t)0)
        KV_SERIALIZE_OPT(ring_size, (uint64_t)0)
        KV_SERIALIZE(unlock_time)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(get_tx_keys)
        KV_SERIALIZE_OPT(do_not_relay, false)
        KV_SERIALIZE_OPT(get_tx_hex, false)
        KV_SERIALIZE_OPT(get_tx_metadata, false)
      END_KV_SERIALIZE_MAP()
    };

    struct key_list
    {
      std::list<std::string> keys;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(keys)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<std::string> tx_hash_list;
      std::list<std::string> tx_key_list;
      std::list<uint64_t> amount_list;
      std::list<uint64_t> fee_list;
      std::list<std::string> tx_blob_list;
      std::list<std::string> tx_metadata_list;
      std::string multisig_txset;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash_list)
        KV_SERIALIZE(tx_key_list)
        KV_SERIALIZE(amount_list)
        KV_SERIALIZE(fee_list)
        KV_SERIALIZE(tx_blob_list)
        KV_SERIALIZE(tx_metadata_list)
        KV_SERIALIZE(multisig_txset)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SWEEP_DUST
  {
    struct request
    {
      bool get_tx_keys;
      bool do_not_relay;
      bool get_tx_hex;
      bool get_tx_metadata;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(get_tx_keys)
        KV_SERIALIZE_OPT(do_not_relay, false)
        KV_SERIALIZE_OPT(get_tx_hex, false)
        KV_SERIALIZE_OPT(get_tx_metadata, false)
      END_KV_SERIALIZE_MAP()
    };

    struct key_list
    {
      std::list<std::string> keys;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(keys)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<std::string> tx_hash_list;
      std::list<std::string> tx_key_list;
      std::list<uint64_t> amount_list;
      std::list<uint64_t> fee_list;
      std::list<std::string> tx_blob_list;
      std::list<std::string> tx_metadata_list;
      std::string multisig_txset;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash_list)
        KV_SERIALIZE(tx_key_list)
        KV_SERIALIZE(amount_list)
        KV_SERIALIZE(fee_list)
        KV_SERIALIZE(tx_blob_list)
        KV_SERIALIZE(tx_metadata_list)
        KV_SERIALIZE(multisig_txset)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SWEEP_ALL
  {
    struct request
    {
      std::string address;
      uint32_t account_index;
      std::set<uint32_t> subaddr_indices;
      uint32_t priority;
      uint64_t mixin;
      uint64_t ring_size;
      uint64_t unlock_time;
      std::string payment_id;
      bool get_tx_keys;
      uint64_t below_amount;
      bool do_not_relay;
      bool get_tx_hex;
      bool get_tx_metadata;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(subaddr_indices)
        KV_SERIALIZE(priority)
        KV_SERIALIZE_OPT(mixin, (uint64_t)0)
        KV_SERIALIZE_OPT(ring_size, (uint64_t)0)
        KV_SERIALIZE(unlock_time)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(get_tx_keys)
        KV_SERIALIZE(below_amount)
        KV_SERIALIZE_OPT(do_not_relay, false)
        KV_SERIALIZE_OPT(get_tx_hex, false)
        KV_SERIALIZE_OPT(get_tx_metadata, false)
      END_KV_SERIALIZE_MAP()
    };

    struct key_list
    {
      std::list<std::string> keys;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(keys)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<std::string> tx_hash_list;
      std::list<std::string> tx_key_list;
      std::list<uint64_t> amount_list;
      std::list<uint64_t> fee_list;
      std::list<std::string> tx_blob_list;
      std::list<std::string> tx_metadata_list;
      std::string multisig_txset;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash_list)
        KV_SERIALIZE(tx_key_list)
        KV_SERIALIZE(amount_list)
        KV_SERIALIZE(fee_list)
        KV_SERIALIZE(tx_blob_list)
        KV_SERIALIZE(tx_metadata_list)
        KV_SERIALIZE(multisig_txset)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SWEEP_SINGLE
  {
    struct request
    {
      std::string address;
      uint32_t priority;
      uint64_t mixin;
      uint64_t ring_size;
      uint64_t unlock_time;
      std::string payment_id;
      bool get_tx_key;
      std::string key_image;
      bool do_not_relay;
      bool get_tx_hex;
      bool get_tx_metadata;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(priority)
        KV_SERIALIZE_OPT(mixin, (uint64_t)0)
        KV_SERIALIZE_OPT(ring_size, (uint64_t)0)
        KV_SERIALIZE(unlock_time)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(get_tx_key)
        KV_SERIALIZE(key_image)
        KV_SERIALIZE_OPT(do_not_relay, false)
        KV_SERIALIZE_OPT(get_tx_hex, false)
        KV_SERIALIZE_OPT(get_tx_metadata, false)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string tx_hash;
      std::string tx_key;
      uint64_t amount;
      uint64_t fee;
      std::string tx_blob;
      std::string tx_metadata;
      std::string multisig_txset;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
        KV_SERIALIZE(tx_key)
        KV_SERIALIZE(amount)
        KV_SERIALIZE(fee)
        KV_SERIALIZE(tx_blob)
        KV_SERIALIZE(tx_metadata)
        KV_SERIALIZE(multisig_txset)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_RELAY_TX
  {
    struct request
    {
      std::string hex;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(hex)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string tx_hash;
      std::string tx_key;
      uint64_t fee;
      std::string tx_blob;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
        KV_SERIALIZE(tx_key)
        KV_SERIALIZE(fee)
        KV_SERIALIZE(tx_blob)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_STORE
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct payment_details
  {
    std::string payment_id;
    std::string tx_hash;
    uint64_t amount;
    uint64_t block_height;
    uint64_t unlock_time;
    cryptonote::subaddress_index subaddr_index;
    std::string address;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(payment_id)
      KV_SERIALIZE(tx_hash)
      KV_SERIALIZE(amount)
      KV_SERIALIZE(block_height)
      KV_SERIALIZE(unlock_time)
      KV_SERIALIZE(subaddr_index)
      KV_SERIALIZE(address)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_GET_PAYMENTS
  {
    struct request
    {
      std::string payment_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payment_id)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<payment_details> payments;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payments)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_BULK_PAYMENTS
  {
    struct request
    {
      std::vector<std::string> payment_ids;
      uint64_t min_block_height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payment_ids)
        KV_SERIALIZE(min_block_height)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<payment_details> payments;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payments)
      END_KV_SERIALIZE_MAP()
    };
  };
  
  struct transfer_details
  {
    uint64_t amount;
    bool spent;
    uint64_t global_index;
    std::string tx_hash;
    uint64_t tx_size;
    uint32_t subaddr_index;
    std::string key_image;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE(spent)
      KV_SERIALIZE(global_index)
      KV_SERIALIZE(tx_hash)
      KV_SERIALIZE(tx_size)
      KV_SERIALIZE(subaddr_index)
      KV_SERIALIZE(key_image)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_INCOMING_TRANSFERS
  {
    struct request
    {
      std::string transfer_type;
      uint32_t account_index;
      std::set<uint32_t> subaddr_indices;
      bool verbose;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(transfer_type)
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(subaddr_indices)
        KV_SERIALIZE(verbose)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<transfer_details> transfers;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(transfers)
      END_KV_SERIALIZE_MAP()
    };
  };

  //JSON RPC V2
  struct COMMAND_RPC_QUERY_KEY
  {
    struct request
    {
      std::string key_type;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key_type)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string key;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_MAKE_INTEGRATED_ADDRESS
  {
    struct request
    {
      std::string payment_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payment_id)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string integrated_address;
      std::string payment_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(integrated_address)
        KV_SERIALIZE(payment_id)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS
  {
    struct request
    {
      std::string integrated_address;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(integrated_address)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string standard_address;
      std::string payment_id;
      bool is_subaddress;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(standard_address)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(is_subaddress)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_STOP_WALLET
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_RESCAN_BLOCKCHAIN
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SET_TX_NOTES
  {
    struct request
    {
      std::list<std::string> txids;
      std::list<std::string> notes;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txids)
        KV_SERIALIZE(notes)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_TX_NOTES
  {
    struct request
    {
      std::list<std::string> txids;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txids)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<std::string> notes;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(notes)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SET_ATTRIBUTE
  {
    struct request
    {
      std::string key;
      std::string value;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key)
        KV_SERIALIZE(value)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_ATTRIBUTE
  {
    struct request
    {

      std::string key;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string value;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(value)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_TX_KEY
  {
    struct request
    {
      std::string txid;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txid)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string tx_key;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_key)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_CHECK_TX_KEY
  {
    struct request
    {
      std::string txid;
      std::string tx_key;
      std::string address;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txid)
        KV_SERIALIZE(tx_key)
        KV_SERIALIZE(address)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t received;
      bool in_pool;
      uint64_t confirmations;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(received)
        KV_SERIALIZE(in_pool)
        KV_SERIALIZE(confirmations)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_TX_PROOF
  {
    struct request
    {
      std::string txid;
      std::string address;
      std::string message;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txid)
        KV_SERIALIZE(address)
        KV_SERIALIZE(message)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string signature;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signature)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_CHECK_TX_PROOF
  {
    struct request
    {
      std::string txid;
      std::string address;
      std::string message;
      std::string signature;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txid)
        KV_SERIALIZE(address)
        KV_SERIALIZE(message)
        KV_SERIALIZE(signature)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      bool good;
      uint64_t received;
      bool in_pool;
      uint64_t confirmations;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(good)
        KV_SERIALIZE(received)
        KV_SERIALIZE(in_pool)
        KV_SERIALIZE(confirmations)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct transfer_entry
  {
    std::string txid;
    std::string payment_id;
    uint64_t height;
    uint64_t timestamp;
    uint64_t amount;
    uint64_t fee;
    std::string note;
    std::list<transfer_destination> destinations;
    std::string type;
    uint64_t unlock_time;
    cryptonote::subaddress_index subaddr_index;
    std::string address;
    bool double_spend_seen;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(txid);
      KV_SERIALIZE(payment_id);
      KV_SERIALIZE(height);
      KV_SERIALIZE(timestamp);
      KV_SERIALIZE(amount);
      KV_SERIALIZE(fee);
      KV_SERIALIZE(note);
      KV_SERIALIZE(destinations);
      KV_SERIALIZE(type);
      KV_SERIALIZE(unlock_time)
      KV_SERIALIZE(subaddr_index);
      KV_SERIALIZE(address);
      KV_SERIALIZE(double_spend_seen)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_GET_SPEND_PROOF
  {
    struct request
    {
      std::string txid;
      std::string message;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txid)
        KV_SERIALIZE(message)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string signature;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signature)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_CHECK_SPEND_PROOF
  {
    struct request
    {
      std::string txid;
      std::string message;
      std::string signature;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txid)
        KV_SERIALIZE(message)
        KV_SERIALIZE(signature)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      bool good;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(good)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_RESERVE_PROOF
  {
    struct request
    {
      bool all;
      uint32_t account_index;     // ignored when `all` is true
      uint64_t amount;            // ignored when `all` is true
      std::string message;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(all)
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(amount)
        KV_SERIALIZE(message)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string signature;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signature)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_CHECK_RESERVE_PROOF
  {
    struct request
    {
      std::string address;
      std::string message;
      std::string signature;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(message)
        KV_SERIALIZE(signature)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      bool good;
      uint64_t total;
      uint64_t spent;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(good)
        KV_SERIALIZE(total)
        KV_SERIALIZE(spent)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_TRANSFERS
  {
    struct request
    {
      bool in;
      bool out;
      bool pending;
      bool failed;
      bool pool;

      bool filter_by_height;
      uint64_t min_height;
      uint64_t max_height;
      uint32_t account_index;
      std::set<uint32_t> subaddr_indices;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(in);
        KV_SERIALIZE(out);
        KV_SERIALIZE(pending);
        KV_SERIALIZE(failed);
        KV_SERIALIZE(pool);
        KV_SERIALIZE(filter_by_height);
        KV_SERIALIZE(min_height);
        KV_SERIALIZE_OPT(max_height, (uint64_t)CRYPTONOTE_MAX_BLOCK_NUMBER);
        KV_SERIALIZE(account_index);
        KV_SERIALIZE(subaddr_indices);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<transfer_entry> in;
      std::list<transfer_entry> out;
      std::list<transfer_entry> pending;
      std::list<transfer_entry> failed;
      std::list<transfer_entry> pool;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(in);
        KV_SERIALIZE(out);
        KV_SERIALIZE(pending);
        KV_SERIALIZE(failed);
        KV_SERIALIZE(pool);
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_TRANSFER_BY_TXID
  {
    struct request
    {
      std::string txid;
      uint32_t account_index;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txid);
        KV_SERIALIZE_OPT(account_index, (uint32_t)0)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      transfer_entry transfer;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(transfer);
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SIGN
  {
    struct request
    {
      std::string data;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(data);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string signature;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signature);
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_VERIFY
  {
    struct request
    {
      std::string data;
      std::string address;
      std::string signature;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(data);
        KV_SERIALIZE(address);
        KV_SERIALIZE(signature);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      bool good;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(good);
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_EXPORT_KEY_IMAGES
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct signed_key_image
    {
      std::string key_image;
      std::string signature;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key_image);
        KV_SERIALIZE(signature);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<signed_key_image> signed_key_images;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signed_key_images);
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_IMPORT_KEY_IMAGES
  {
    struct signed_key_image
    {
      std::string key_image;
      std::string signature;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key_image);
        KV_SERIALIZE(signature);
      END_KV_SERIALIZE_MAP()
    };

    struct request
    {
      std::vector<signed_key_image> signed_key_images;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signed_key_images);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t height;
      uint64_t spent;
      uint64_t unspent;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height)
        KV_SERIALIZE(spent)
        KV_SERIALIZE(unspent)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct uri_spec
  {
    std::string address;
    std::string payment_id;
    uint64_t amount;
    std::string tx_description;
    std::string recipient_name;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(address);
      KV_SERIALIZE(payment_id);
      KV_SERIALIZE(amount);
      KV_SERIALIZE(tx_description);
      KV_SERIALIZE(recipient_name);
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_MAKE_URI
  {
    struct request: public uri_spec
    {
    };

    struct response
    {
      std::string uri;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(uri)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_PARSE_URI
  {
    struct request
    {
      std::string uri;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(uri)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uri_spec uri;
      std::vector<std::string> unknown_parameters;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(uri);
        KV_SERIALIZE(unknown_parameters);
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_ADD_ADDRESS_BOOK_ENTRY
  {
    struct request
    {
      std::string address;
      std::string payment_id;
      std::string description;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(description)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t index;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(index);
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_ADDRESS_BOOK_ENTRY
  {
    struct request
    {
      std::list<uint64_t> entries;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(entries)
      END_KV_SERIALIZE_MAP()
    };

    struct entry
    {
      uint64_t index;
      std::string address;
      std::string payment_id;
      std::string description;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(index)
        KV_SERIALIZE(address)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(description)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<entry> entries;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(entries)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_DELETE_ADDRESS_BOOK_ENTRY
  {
    struct request
    {
      uint64_t index;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(index);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_RESCAN_SPENT
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_START_MINING
  {
    struct request
    {
      uint64_t    threads_count;
      bool        do_background_mining;
      bool        ignore_battery;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(threads_count)
        KV_SERIALIZE(do_background_mining)        
        KV_SERIALIZE(ignore_battery)        
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_STOP_MINING
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_LANGUAGES
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    struct response
    {
      std::vector<std::string> languages;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(languages)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_CREATE_WALLET
  {
    struct request
    {
      std::string filename;
      std::string password;
      std::string language;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(filename)
        KV_SERIALIZE(password)
        KV_SERIALIZE(language)
      END_KV_SERIALIZE_MAP()
    };
    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_OPEN_WALLET
  {
    struct request
    {
      std::string filename;
      std::string password;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(filename)
        KV_SERIALIZE(password)
      END_KV_SERIALIZE_MAP()
    };
    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_IS_MULTISIG
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      bool multisig;
      bool ready;
      uint32_t threshold;
      uint32_t total;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(multisig)
        KV_SERIALIZE(ready)
        KV_SERIALIZE(threshold)
        KV_SERIALIZE(total)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_PREPARE_MULTISIG
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string multisig_info;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(multisig_info)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_MAKE_MULTISIG
  {
    struct request
    {
      std::vector<std::string> multisig_info;
      uint32_t threshold;
      std::string password;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(multisig_info)
        KV_SERIALIZE(threshold)
        KV_SERIALIZE(password)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string address;
      std::string multisig_info;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(multisig_info)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_EXPORT_MULTISIG
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string info;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(info)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_IMPORT_MULTISIG
  {
    struct request
    {
      std::vector<std::string> info;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(info)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t n_outputs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(n_outputs)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_FINALIZE_MULTISIG
  {
    struct request
    {
      std::string password;
      std::vector<std::string> multisig_info;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(password)
        KV_SERIALIZE(multisig_info)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string address;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SIGN_MULTISIG
  {
    struct request
    {
      std::string tx_data_hex;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_data_hex)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string tx_data_hex;
      std::list<std::string> tx_hash_list;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_data_hex)
        KV_SERIALIZE(tx_hash_list)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SUBMIT_MULTISIG
  {
    struct request
    {
      std::string tx_data_hex;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_data_hex)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<std::string> tx_hash_list;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash_list)
      END_KV_SERIALIZE_MAP()
    };
  };

}
}
