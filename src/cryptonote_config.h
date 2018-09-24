// Copyright (c) 2017-2018, Sumokoin Project
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

#include <string>
#include <boost/uuid/uuid.hpp>

#define CRYPTONOTE_DNS_TIMEOUT_MS                       20000

#define CRYPTONOTE_MAX_BLOCK_NUMBER                     500000000
#define CRYPTONOTE_MAX_BLOCK_SIZE                       500000000  // block header blob limit, never used!
#define CRYPTONOTE_GETBLOCKTEMPLATE_MAX_BLOCK_SIZE	    196608 //size of block (bytes) that is the maximum that miners will produce
#define CRYPTONOTE_MAX_TX_SIZE                          1000000000
#define CRYPTONOTE_PUBLIC_ADDRESS_TEXTBLOB_VER          0
#define CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW            60
#define CURRENT_BLOCK_MAJOR_VERSION                     1
#define CURRENT_BLOCK_MINOR_VERSION                     0
#define CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE             10

#define CURRENT_TRANSACTION_VERSION                     2
#define MIN_TRANSACTION_VERSION							            2
#define CRYPTONOTE_V2_POW_BLOCK_VERSION                 3
#define CRYPTONOTE_CLASSIC_POW_BLOCK_VERSION            4
#define CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT              60*60*2
#define CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE             10
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
#define CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE          600
#define CRYPTONOTE_DISPLAY_DECIMAL_POINT                9

// COIN - number of smallest units in one coin
#define COIN                                            ((uint64_t)1000000000) // pow(10, 9)

#define FEE_PER_KB                                      ((uint64_t)500000) 
#define DYNAMIC_FEE_PER_KB_BASE_FEE                     ((uint64_t)500000) // 0.0005 * pow(10, 9)
#define DYNAMIC_FEE_PER_KB_BASE_BLOCK_REWARD            ((uint64_t)64000000000) // 64 * pow(10, 9)

#define ORPHANED_BLOCKS_MAX_COUNT                       100

#define DIFFICULTY_TARGET                               240  // seconds
#define DIFFICULTY_WINDOW                               720  // blocks
#define DIFFICULTY_LAG                                  15   // !!!
#define DIFFICULTY_CUT                                  60   // timestamps to cut after sorting
#define DIFFICULTY_BLOCKS_COUNT                         DIFFICULTY_WINDOW + DIFFICULTY_LAG

#define DIFFICULTY_WINDOW_V2							              17
#define DIFFICULTY_CUT_V2                               6
#define DIFFICULTY_BLOCKS_COUNT_V2                      DIFFICULTY_WINDOW_V2 + DIFFICULTY_CUT_V2*2

#define DIFFICULTY_WINDOW_V3                            60
#define DIFFICULTY_BLOCKS_COUNT_V3                      DIFFICULTY_WINDOW_V3 + 1

#define CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS       1
#define CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS      DIFFICULTY_TARGET * CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS

#define DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN             DIFFICULTY_TARGET //just alias; used by tests

#define BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT          10000  //by default, blocks ids count in synchronizing
#define BLOCKS_SYNCHRONIZING_DEFAULT_COUNT_PRE_V4       100    //by default, blocks count in blocks downloading
#define BLOCKS_SYNCHRONIZING_DEFAULT_COUNT              20     //by default, blocks count in blocks downloading

#define CRYPTONOTE_MEMPOOL_TX_LIVETIME                    (86400*3) //seconds, three days
#define CRYPTONOTE_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME     604800 //seconds, one week

#define COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT           1000

#define P2P_LOCAL_WHITE_PEERLIST_LIMIT                  1000
#define P2P_LOCAL_GRAY_PEERLIST_LIMIT                   5000

#define P2P_DEFAULT_CONNECTIONS_COUNT                   8
#define P2P_DEFAULT_HANDSHAKE_INTERVAL                  60           //secondes
#define P2P_DEFAULT_PACKET_MAX_SIZE                     50000000     //50000000 bytes maximum packet size
#define P2P_DEFAULT_PEERS_IN_HANDSHAKE                  250
#define P2P_DEFAULT_CONNECTION_TIMEOUT                  5000       //5 seconds
#define P2P_DEFAULT_PING_CONNECTION_TIMEOUT             2000       //2 seconds
#define P2P_DEFAULT_INVOKE_TIMEOUT                      60*2*1000  //2 minutes
#define P2P_DEFAULT_HANDSHAKE_INVOKE_TIMEOUT            5000       //5 seconds
#define P2P_DEFAULT_WHITELIST_CONNECTIONS_PERCENT       70
#define P2P_DEFAULT_ANCHOR_CONNECTIONS_COUNT            2

#define P2P_FAILED_ADDR_FORGET_SECONDS                  (60*60)     //1 hour
#define P2P_IP_BLOCKTIME                                (60*60*24)  //24 hour
#define P2P_IP_FAILS_BEFORE_BLOCK                       10
#define P2P_IDLE_CONNECTION_KILL_INTERVAL               (5*60) //5 minutes

#define P2P_SUPPORT_FLAG_FLUFFY_BLOCKS                  0x01
#define P2P_SUPPORT_FLAGS                               P2P_SUPPORT_FLAG_FLUFFY_BLOCKS

#define ALLOW_DEBUG_COMMANDS

#define CRYPTONOTE_NAME                         "sumokoin"
#define CRYPTONOTE_POOLDATA_FILENAME            "poolstate.bin"
#define CRYPTONOTE_BLOCKCHAINDATA_FILENAME      "data.mdb"
#define CRYPTONOTE_BLOCKCHAINDATA_LOCK_FILENAME "lock.mdb"
#define P2P_NET_DATA_FILENAME                   "p2pstate.bin"
#define MINER_CONFIG_FILE_NAME                  "miner_conf.json"

#define THREAD_STACK_SIZE                       5 * 1024 * 1024

// coin emission change interval/speed configs
#define COIN_EMISSION_MONTH_INTERVAL                    6  // months to change emission speed
#define COIN_EMISSION_HEIGHT_INTERVAL                   ((uint64_t) (COIN_EMISSION_MONTH_INTERVAL * (30.4375 * 24 * 3600) / DIFFICULTY_TARGET)) // calculated to # of heights to change emission speed
#define PEAK_COIN_EMISSION_YEAR                         4
#define PEAK_COIN_EMISSION_HEIGHT                       ((uint64_t) (((12 * 30.4375 * 24 * 3600)/DIFFICULTY_TARGET) * PEAK_COIN_EMISSION_YEAR)) // = (# of heights emmitted per year) * PEAK_COIN_EMISSION_YEAR

#define DEFAULT_MIXIN                                   12     // default & minimum mixin allowed
#define MAX_MIXIN                                       240    

#define TRANSACTION_SIZE_LIMIT                          ((uint64_t) ((CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 110 / 100) - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE))
#define BLOCK_SIZE_GROWTH_FAVORED_ZONE                  ((uint64_t) (CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 4))

#define PER_KB_FEE_QUANTIZATION_DECIMALS        6

#define HASH_OF_HASHES_STEP                     256

#define DEFAULT_TXPOOL_MAX_SIZE                 259200000ull // 3 days at 240000, in bytes

#define BULLETPROOF_HF_VERSION                  ((uint8_t) 10) // correct this when bf enabled

// New constants are intended to go here
namespace config
{
  uint64_t const DEFAULT_FEE_ATOMIC_XMR_PER_KB = 500; // Just a placeholder!  Change me!
  uint8_t const FEE_CALCULATION_MAX_RETRIES = 10;
  uint64_t const DEFAULT_DUST_THRESHOLD = ((uint64_t)10000000); // pow(10, 7)
  uint64_t const BASE_REWARD_CLAMP_THRESHOLD = ((uint64_t)100000); // pow(10, 5)
  std::string const P2P_REMOTE_DEBUG_TRUSTED_PUB_KEY = "0000000000000000000000000000000000000000000000000000000000000000";

  uint64_t const CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = 0x2bb39a;  // Sumo
  uint64_t const CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX = 0x29339a; //Sumi
  uint64_t const CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX = 0x8319a; // Subo
  uint16_t const P2P_DEFAULT_PORT = 19733;
  uint16_t const RPC_DEFAULT_PORT = 19734;
  uint16_t const ZMQ_RPC_DEFAULT_PORT = 19735;

  boost::uuids::uuid const NETWORK_ID = { {
      0x04, 0x06, 0xdf, 0xce, 0xfc, 0x7c, 0x27, 0x4a, 0x24, 0xd4, 0xf3, 0x8d, 0x42, 0x44, 0x60, 0xa8
    } }; // Bender's nightmare
  std::string const GENESIS_TX = "023c01ff0001808098d0daf1d00f028be379aa57a70fa19c0ee5765fdc3d2aae0b1034158f4963e157d9042c24fbec21013402fc7071230f1f86f33099119105a7b1f64a898526060ab871e685059c223100";
  uint32_t const GENESIS_NONCE = 10000;
  
  // Funding for exchange burned coins
  static constexpr const char* EXCHANGE_FUND_ADDRESS = "Sumoo2y7AAteNGJ5yepUUFX3taqDRfM5eYHmCc1qnhwx6cJp3htcjTbKWH7NxkcADcT82pRwns9Us7NdCmuw3gx8bnzYGg14L2o";
  // aa0d3ec96e05fd154235fdcff4c2ebce79f49f4ce76f0aeabeb4a40cf64cc30b
  static constexpr const char* EXCHANGE_FUND_VIEWKEY = "\xaa\x0d\x3e\xc9\x6e\x05\xfd\x15\x42\x35\xfd\xcf\xf4\xc2\xeb\xce\x79\xf4\x9f\x4c\xe7\x6f\x0a\xea\xbe\xb4\xa4\x0c\xf6\x4c\xc3\x0b";
  // Number of coins burned
  static constexpr uint64_t EXCHANGE_BURNED_AMOUNT = 1044138500000000;   // 1,044,138.5 coins burned
  static constexpr uint64_t EXCHANGE_FUND_AMOUNT = EXCHANGE_BURNED_AMOUNT; // fund amount = burned coins
  static constexpr uint64_t EXCHANGE_FUND_RELEASE_HEIGHT = 199800;  // equal v6 hardfork
    
  namespace testnet
  {
    uint64_t const CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = 0x37751a; // Suto
    uint64_t const CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX = 0x34f51a; // Suti
    uint64_t const CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX = 0x1d351a; // Susu
    uint16_t const P2P_DEFAULT_PORT = 29733;
    uint16_t const RPC_DEFAULT_PORT = 29734;
    uint16_t const ZMQ_RPC_DEFAULT_PORT = 29735;
    boost::uuids::uuid const NETWORK_ID = { {
        0x12, 0x04, 0x06, 0xdf, 0xce, 0xfc, 0x7c, 0x27, 0x4a, 0x24, 0xd4, 0xf3, 0x8d, 0x42, 0x44, 0x60
      } }; // Bender's daydream
    std::string const GENESIS_TX = "023c01ff0001808098d0daf1d00f028be379aa57a70fa19c0ee5765fdc3d2aae0b1034158f4963e157d9042c24fbec21013402fc7071230f1f86f33099119105a7b1f64a898526060ab871e685059c223100";
    uint32_t const GENESIS_NONCE = 10001;
  }

  namespace stagenet
  {
    uint64_t const CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = 0x37751a; // Suto
    uint64_t const CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX = 0x34f51a; // Suti
    uint64_t const CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX = 0x1d351a; // Susu
    uint16_t const P2P_DEFAULT_PORT = 39733;
    uint16_t const RPC_DEFAULT_PORT = 39734;
    uint16_t const ZMQ_RPC_DEFAULT_PORT = 39735;
    boost::uuids::uuid const NETWORK_ID = { {
        0x12, 0x04, 0x06, 0xdf, 0xce, 0xfc, 0x7c, 0x27, 0x4a, 0x24, 0xd4, 0xf3, 0x8d, 0x42, 0x44, 0x60
      } }; // Bender's daydream
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
}
