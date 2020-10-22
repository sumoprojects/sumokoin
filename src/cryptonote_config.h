// Copyright (c) 2017-2020, Sumokoin Project
// Copyright (c) 2014-2020, The Monero Project
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

#include <stdexcept>
#include <string>
#include <boost/uuid/uuid.hpp>
#include "p2p_config.h"
#include "crypto_config.h"
#include "seeds_config.h"
#include "hardforks_checkpoints_config.h"
#include "difficulty_config.h"

#define CRYPTONOTE_MAX_BLOCK_NUMBER                     500000000
#define CRYPTONOTE_MAX_BLOCK_WEIGHT                     500000000  // block header blob limit, never used!
#define CRYPTONOTE_MAX_TX_SIZE                          1000000000
#define CRYPTONOTE_MAX_TX_PER_BLOCK                     0x10000000
#define CRYPTONOTE_PUBLIC_ADDRESS_TEXTBLOB_VER          0
#define CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW            60
#define CURRENT_BLOCK_MAJOR_VERSION                     1
#define CURRENT_BLOCK_MINOR_VERSION                     0

#define CURRENT_TRANSACTION_VERSION                     2
#define MIN_TRANSACTION_VERSION                         2
#define CRYPTONOTE_HEAVY_BLOCK_VERSION                  3

#define CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE             10

#define CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT              60*60*2
#define BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW               60
#define CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT_V2           60*24
#define BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW_V2            12
#define CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT_V3           60*5
#define BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW_V3            BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW_V2

// MONEY_SUPPLY - total number coins to be generated
#define MONEY_SUPPLY                                    ((uint64_t)88888888000000000)
#define EMISSION_SPEED_FACTOR                           19
#define FINAL_SUBSIDY                                   ((uint64_t)4000000000) // 4 * pow(10, 9)
#define GENESIS_BLOCK_REWARD                            ((uint64_t)8800000000000000) // ~10% dev premine

#define CRYPTONOTE_REWARD_BLOCKS_WINDOW                 60
#define CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE       240 * 1024    // 240kB
#define CRYPTONOTE_LONG_TERM_BLOCK_WEIGHT_WINDOW_SIZE   100000 // size in blocks of the long term block weight median window
#define CRYPTONOTE_SHORT_TERM_BLOCK_WEIGHT_SURGE_FACTOR 50
#define CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE          600
#define CRYPTONOTE_DISPLAY_DECIMAL_POINT                9

// COIN - number of smallest units in one coin
#define COIN                                            ((uint64_t)1000000000) // pow(10, 9)

#define FEE_PER_KB                                      ((uint64_t)500000)
#define FEE_PER_BYTE                                    ((uint64_t)500)
#define DYNAMIC_FEE_PER_KB_BASE_FEE                     ((uint64_t)500000) // 0.0005 * pow(10, 9)
#define DYNAMIC_FEE_PER_KB_BASE_BLOCK_REWARD            ((uint64_t)64000000000) // 64 * pow(10, 9)
#define DYNAMIC_FEE_REFERENCE_TRANSACTION_WEIGHT        ((uint64_t)5000)

#define ORPHANED_BLOCKS_MAX_COUNT                       100

#define CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS       1
#define CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS      DIFFICULTY_TARGET * CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS

#define CRYPTONOTE_MEMPOOL_TX_LIVETIME                  (86400*3) //seconds, three days
#define CRYPTONOTE_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME   604800 //seconds, one week

#define CRYPTONOTE_NAME                                 "sumokoin"
#define CRYPTONOTE_BLOCKCHAINDATA_FILENAME              "data.mdb"
#define CRYPTONOTE_BLOCKCHAINDATA_LOCK_FILENAME         "lock.mdb"
#define P2P_NET_DATA_FILENAME                           "p2pstate.bin"
#define RPC_PAYMENTS_DATA_FILENAME                      "rpcpayments.bin"
#define MINER_CONFIG_FILE_NAME                          "miner_conf.json"

// coin emission change interval/speed configs
#define COIN_EMISSION_MONTH_INTERVAL                    6  // months to change emission speed
#define COIN_EMISSION_HEIGHT_INTERVAL                   ((uint64_t) (COIN_EMISSION_MONTH_INTERVAL * (30.4375 * 24 * 3600) / DIFFICULTY_TARGET)) // calculated to # of heights to change emission speed
#define PEAK_COIN_EMISSION_YEAR                         4
#define PEAK_COIN_EMISSION_HEIGHT                       ((uint64_t) (((12 * 30.4375 * 24 * 3600)/DIFFICULTY_TARGET) * PEAK_COIN_EMISSION_YEAR)) // = (# of heights emmitted per year) * PEAK_COIN_EMISSION_YEAR

#define DEFAULT_MIXIN                                   12     // default & minimum mixin allowed
#define MAX_MIXIN                                       240
#define DEFAULT_MIXIN_V2                                48

#define TRANSACTION_WEIGHT_LIMIT                        ((uint64_t) ((CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 110 / 100) - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE))
#define BLOCK_SIZE_GROWTH_FAVORED_ZONE                  ((uint64_t) (CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 4))

#define PER_KB_FEE_QUANTIZATION_DECIMALS                 6
#define HASH_OF_HASHES_STEP                              512
#define BULLETPROOF_MAX_OUTPUTS                          16
#define DEFAULT_TXPOOL_MAX_WEIGHT                        259200000ull // 3 days at 240000, in bytes

#define CRYPTONOTE_PRUNING_STRIPE_SIZE                   4096 // the smaller, the smoother the increase
#define CRYPTONOTE_PRUNING_LOG_STRIPES                   3 // the higher, the more space saved
#define CRYPTONOTE_PRUNING_TIP_BLOCKS                    5500 // the smaller, the more space saved

#define RPC_CREDITS_PER_HASH_SCALE                       ((float)(1<<24))

namespace config
{
  uint64_t const DEFAULT_FEE_ATOMIC_XMR_PER_KB = 500; // Just a placeholder!  Change me!
  uint8_t const FEE_CALCULATION_MAX_RETRIES = 10;
  uint64_t const DEFAULT_DUST_THRESHOLD = ((uint64_t)10000000); // pow(10, 7)
  uint64_t const BASE_REWARD_CLAMP_THRESHOLD = ((uint64_t)100000); // pow(10, 5)

  // Funding for exchange burned coins
  static constexpr const char* EXCHANGE_FUND_ADDRESS = "Sumoo2y7AAteNGJ5yepUUFX3taqDRfM5eYHmCc1qnhwx6cJp3htcjTbKWH7NxkcADcT82pRwns9Us7NdCmuw3gx8bnzYGg14L2o";
  // aa0d3ec96e05fd154235fdcff4c2ebce79f49f4ce76f0aeabeb4a40cf64cc30b
  static constexpr const char* EXCHANGE_FUND_VIEWKEY = "\xaa\x0d\x3e\xc9\x6e\x05\xfd\x15\x42\x35\xfd\xcf\xf4\xc2\xeb\xce\x79\xf4\x9f\x4c\xe7\x6f\x0a\xea\xbe\xb4\xa4\x0c\xf6\x4c\xc3\x0b";
  // Number of coins burned
  static constexpr uint64_t EXCHANGE_BURNED_AMOUNT = 1044138500000000;   // 1,044,138.5 coins burned
  static constexpr uint64_t EXCHANGE_FUND_AMOUNT = EXCHANGE_BURNED_AMOUNT; // fund amount = burned coins
  static constexpr uint64_t EXCHANGE_FUND_RELEASE_HEIGHT = 199800;  // equal v6 hardfork

  uint64_t const CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = 0x2bb39a;  // Sumo
  uint64_t const CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX = 0x29339a; //Sumi
  uint64_t const CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX = 0x8319a; // Subo
  std::string const GENESIS_TX = "023c01ff0001808098d0daf1d00f028be379aa57a70fa19c0ee5765fdc3d2aae0b1034158f4963e157d9042c24fbec21013402fc7071230f1f86f33099119105a7b1f64a898526060ab871e685059c223100";
  uint32_t const GENESIS_NONCE = 10000;

  namespace testnet
  {
    uint64_t const CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = 0x37751a; // Suto
    uint64_t const CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX = 0x34f51a; // Suti
    uint64_t const CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX = 0x1d351a; // Susu
    std::string const GENESIS_TX = "023c01ff0001808098d0daf1d00f028be379aa57a70fa19c0ee5765fdc3d2aae0b1034158f4963e157d9042c24fbec21013402fc7071230f1f86f33099119105a7b1f64a898526060ab871e685059c223100";
    uint32_t const GENESIS_NONCE = 10001;
  }

  namespace stagenet
  {
    uint64_t const CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = 0x37751a; // Suto
    uint64_t const CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX = 0x34f51a; // Suti
    uint64_t const CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX = 0x1d351a; // Susu
    std::string const GENESIS_TX = "023c01ff0001808098d0daf1d00f028d7bbb5a23ab085e05230bd45d938c71f669f94c2a170b96b64827b7bc2cbde521012b6ed837a56ef72f57b4e46410bdea82e382c43ae8797a0f3e8419b5d4f8e6fe00";
    uint32_t const GENESIS_NONCE = 10002;
  }
}

namespace cryptonote
{
  enum network_type : uint8_t
  {
    MAINNET = 0,
    TESTNET,
    STAGENET,
    FAKECHAIN,
    UNDEFINED = 255
  };
  struct config_t
  {
    uint64_t const CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX;
    uint64_t const CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX;
    uint64_t const CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX;
    uint16_t const P2P_DEFAULT_PORT;
    uint16_t const RPC_DEFAULT_PORT;
    uint16_t const ZMQ_RPC_DEFAULT_PORT;
    boost::uuids::uuid const NETWORK_ID;
    std::string const GENESIS_TX;
    uint32_t const GENESIS_NONCE;
  };
  inline const config_t& get_config(network_type nettype)
  {
    static const config_t mainnet = {
      ::config::CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX,
      ::config::CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX,
      ::config::CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX,
      ::config::P2P_DEFAULT_PORT,
      ::config::RPC_DEFAULT_PORT,
      ::config::ZMQ_RPC_DEFAULT_PORT,
      ::config::NETWORK_ID,
      ::config::GENESIS_TX,
      ::config::GENESIS_NONCE
    };
    static const config_t testnet = {
      ::config::testnet::CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX,
      ::config::testnet::CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX,
      ::config::testnet::CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX,
      ::config::testnet::P2P_DEFAULT_PORT,
      ::config::testnet::RPC_DEFAULT_PORT,
      ::config::testnet::ZMQ_RPC_DEFAULT_PORT,
      ::config::testnet::NETWORK_ID,
      ::config::testnet::GENESIS_TX,
      ::config::testnet::GENESIS_NONCE
    };
    static const config_t stagenet = {
      ::config::stagenet::CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX,
      ::config::stagenet::CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX,
      ::config::stagenet::CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX,
      ::config::stagenet::P2P_DEFAULT_PORT,
      ::config::stagenet::RPC_DEFAULT_PORT,
      ::config::stagenet::ZMQ_RPC_DEFAULT_PORT,
      ::config::stagenet::NETWORK_ID,
      ::config::stagenet::GENESIS_TX,
      ::config::stagenet::GENESIS_NONCE
    };
    switch (nettype)
    {
      case MAINNET: return mainnet;
      case TESTNET: return testnet;
      case STAGENET: return stagenet;
      case FAKECHAIN: return mainnet;
      default: throw std::runtime_error("Invalid network type");
    }
  }
}
