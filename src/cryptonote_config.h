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

#define CRYPTONOTE_DNS_TIMEOUT_MS                       20000

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

#define DIFFICULTY_TARGET                               240  // seconds
#define DIFFICULTY_WINDOW                               720  // blocks
#define DIFFICULTY_LAG                                  15   // !!!
#define DIFFICULTY_CUT                                  60   // timestamps to cut after sorting
#define DIFFICULTY_BLOCKS_COUNT                         DIFFICULTY_WINDOW + DIFFICULTY_LAG

#define DIFFICULTY_WINDOW_V2                            17
#define DIFFICULTY_CUT_V2                               6
#define DIFFICULTY_BLOCKS_COUNT_V2                      DIFFICULTY_WINDOW_V2 + DIFFICULTY_CUT_V2*2

#define DIFFICULTY_WINDOW_V3                            60
#define DIFFICULTY_BLOCKS_COUNT_V3                      DIFFICULTY_WINDOW_V3 + 1

#define CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS       1
#define CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS      DIFFICULTY_TARGET * CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS

#define DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN             DIFFICULTY_TARGET //just alias; used by tests

#define BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT          10000  //by default, blocks ids count in synchronizing
#define BLOCKS_SYNCHRONIZING_DEFAULT_COUNT              10     //by default, blocks count in blocks downloading

#define CRYPTONOTE_MEMPOOL_TX_LIVETIME                  (86400*3) //seconds, three days
#define CRYPTONOTE_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME   604800 //seconds, one week

#define CRYPTONOTE_DANDELIONPP_STEMS                    2 // number of outgoing stem connections per epoch
#define CRYPTONOTE_DANDELIONPP_FLUFF_PROBABILITY        10 // out of 100
#define CRYPTONOTE_DANDELIONPP_MIN_EPOCH                10 // minutes
#define CRYPTONOTE_DANDELIONPP_EPOCH_RANGE              30 // seconds
#define CRYPTONOTE_DANDELIONPP_FLUSH_AVERAGE            5 // seconds average for poisson distributed fluff flush
#define CRYPTONOTE_DANDELIONPP_EMBARGO_AVERAGE          173 // seconds (see tx_pool.cpp for more info)

// see src/cryptonote_protocol/levin_notify.cpp
#define CRYPTONOTE_NOISE_MIN_EPOCH                      5      // minutes
#define CRYPTONOTE_NOISE_EPOCH_RANGE                    30     // seconds
#define CRYPTONOTE_NOISE_MIN_DELAY                      10     // seconds
#define CRYPTONOTE_NOISE_DELAY_RANGE                    5      // seconds
#define CRYPTONOTE_NOISE_BYTES                          3*1024 // 3 KiB
#define CRYPTONOTE_NOISE_CHANNELS                       2      // Max outgoing connections per zone used for noise/covert sending

// Both below are in seconds. The idea is to delay forwarding from i2p/tor
// to ipv4/6, such that 2+ incoming connections _could_ have sent the tx
#define CRYPTONOTE_FORWARD_DELAY_BASE (CRYPTONOTE_NOISE_MIN_DELAY + CRYPTONOTE_NOISE_DELAY_RANGE)
#define CRYPTONOTE_FORWARD_DELAY_AVERAGE (CRYPTONOTE_FORWARD_DELAY_BASE + (CRYPTONOTE_FORWARD_DELAY_BASE / 2))

#define CRYPTONOTE_MAX_FRAGMENTS                        20 // ~20 * NOISE_BYTES max payload size for covert/noise send

#define COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT           1000

#define P2P_LOCAL_WHITE_PEERLIST_LIMIT                  1000
#define P2P_LOCAL_GRAY_PEERLIST_LIMIT                   5000

#define P2P_DEFAULT_CONNECTIONS_COUNT_OUT               8
#define P2P_DEFAULT_CONNECTIONS_COUNT_IN                32
#define P2P_DEFAULT_HANDSHAKE_INTERVAL                  60           //seconds
#define P2P_DEFAULT_PACKET_MAX_SIZE                     50000000     //50000000 bytes maximum packet size
#define P2P_DEFAULT_PEERS_IN_HANDSHAKE                  150
#define P2P_DEFAULT_CONNECTION_TIMEOUT                  5000       //5 seconds
#define P2P_DEFAULT_SOCKS_CONNECT_TIMEOUT               45         // seconds
#define P2P_DEFAULT_PING_CONNECTION_TIMEOUT             5000       //5 seconds
#define P2P_DEFAULT_INVOKE_TIMEOUT                      60*2*1000  //2 minutes
#define P2P_DEFAULT_HANDSHAKE_INVOKE_TIMEOUT            5000       //5 seconds
#define P2P_DEFAULT_WHITELIST_CONNECTIONS_PERCENT       75
#define P2P_DEFAULT_ANCHOR_CONNECTIONS_COUNT            4
#define P2P_DEFAULT_SYNC_SEARCH_CONNECTIONS_COUNT       2
#define P2P_DEFAULT_LIMIT_RATE_UP                       2048       // kB/s
#define P2P_DEFAULT_LIMIT_RATE_DOWN                     8192       // kB/s

#define P2P_FAILED_ADDR_FORGET_SECONDS                  (24*60*60)     //a day
#define P2P_IP_BLOCKTIME                                (2*60*60*24)  //two days
#define P2P_IP_FAILS_BEFORE_BLOCK                       5
#define P2P_IDLE_CONNECTION_KILL_INTERVAL               (30) //30 seconds

#define P2P_SUPPORT_FLAG_FLUFFY_BLOCKS                  0x01
#define P2P_SUPPORT_FLAGS                               P2P_SUPPORT_FLAG_FLUFFY_BLOCKS

#define RPC_IP_FAILS_BEFORE_BLOCK                       3

#define CRYPTONOTE_NAME                                 "sumokoin"
#define CRYPTONOTE_BLOCKCHAINDATA_FILENAME              "data.mdb"
#define CRYPTONOTE_BLOCKCHAINDATA_LOCK_FILENAME         "lock.mdb"
#define P2P_NET_DATA_FILENAME                           "p2pstate.bin"
#define RPC_PAYMENTS_DATA_FILENAME                      "rpcpayments.bin"
#define MINER_CONFIG_FILE_NAME                          "miner_conf.json"

// seed nodes
// MAINNET
#define SEED_MAINNET_1   "144.217.164.165:19733" // Canada
#define SEED_MAINNET_2   "217.182.76.94:19733" // Poland
#define SEED_MAINNET_3   "46.105.92.108:19733" // France
#define SEED_MAINNET_4   "139.99.193.21:19733" // Australia
#define SEED_MAINNET_5   "139.99.40.69:19733" // Singapore
#define SEED_MAINNET_6   "133.18.53.223:19733" // Japan
#define SEED_MAINNET_7   "168.119.105.217:19733" // Germany - explorer
#define SEED_MAINNET_8   "135.181.96.238:19733" // Finland
// TESTNET
#define SEED_TESTNET_1 	 "144.217.164.165:29733"
#define SEED_TESTNET_2 	 "217.182.76.94:29733"
#define SEED_TESTNET_3 	 "139.99.40.69:29733"
#define SEED_TESTNET_4 	 "46.105.92.108:29733"
#define SEED_TESTNET_5 	 "78.47.33.34:29733" // Testnet Explorer
// STAGENET
#define SEED_STAGENET_1  "144.217.164.165:39733"
#define SEED_STAGENET_2  "217.182.76.94:39733"
#define SEED_STAGENET_3  "139.99.40.69:39733"
#define SEED_STAGENET_4  "46.105.92.108:39733"

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


#define HF_VERSION_DYNAMIC_FEE                            1
#define HF_VERSION_PER_BYTE_FEE                           7
#define HF_VERSION_BP                                     7   // Bulletproof HF
#define HF_VERSION_SMALLER_BP                             7
#define HF_VERSION_LONG_TERM_BLOCK_WEIGHT                 8
#define HF_VERSION_MIN_2_OUTPUTS                          9
#define HF_VERSION_ENFORCE_MIN_AGE                        9
#define HF_VERSION_EFFECTIVE_SHORT_TERM_MEDIAN_IN_PENALTY 9
#define HF_VERSION_EXACT_COINBASE                         10
#define HF_VERSION_CLSAG                                  10
#define HF_VERSION_DETERMINISTIC_UNLOCK_TIME              10

#define PER_KB_FEE_QUANTIZATION_DECIMALS                 6
#define HASH_OF_HASHES_STEP                              512
#define BULLETPROOF_MAX_OUTPUTS                          16
#define DEFAULT_TXPOOL_MAX_WEIGHT                        259200000ull // 3 days at 240000, in bytes

#define CRYPTONOTE_PRUNING_STRIPE_SIZE                   4096 // the smaller, the smoother the increase
#define CRYPTONOTE_PRUNING_LOG_STRIPES                   3 // the higher, the more space saved
#define CRYPTONOTE_PRUNING_TIP_BLOCKS                    5500 // the smaller, the more space saved

#define RPC_CREDITS_PER_HASH_SCALE                       ((float)(1<<24))

// mainnet checkpoints
#define MAINNET_CHECKPOINTS \
    ADD_CHECKPOINT(1, "82e8f378ea29d152146b6317903249751b809e97c0b6655f86e120b9de95c38a"); \
    ADD_CHECKPOINT(10, "e097b62bba41e5fd583d3a68de074cddd77c85a6158b031d963232146494a2d6"); \
    ADD_CHECKPOINT(100, "f3bd44c626cc12d449183ca84b58615d792523ba229385ff6717ab29a3e88926"); \
    ADD_CHECKPOINT(1000, "d284c992cb570f86c2e0bcfaa552b1d73bd40417e1c2a40f82bc6432217f0873"); \
    ADD_CHECKPOINT(3000, "81e040955b710dc5a5056668c4eaf3fbc4da2f72c0a63763250ede32a92e0f06"); \
    ADD_CHECKPOINT(5000, "e838c077bc66356d9bb321d4eb60f0851ef766f0619ddc4c6568a0f149aacea0"); \
    ADD_CHECKPOINT(10000, "360b96c3d0a5202c548672d550700d982ca15ad5627f70bce0a89dda840b3611"); \
    ADD_CHECKPOINT(20000, "603a45b60dd92ef4524c80d58411d09480b4668c54bc08dd651d838832bd399e"); \
    ADD_CHECKPOINT(21300, "d0a76e98ebb4d8e928d931a1807bba11a2fafdf544c119761b0ed6de4e1898cf"); \
    ADD_CHECKPOINT(50000, "ae36641cf06ed788375bfb32f0038cbcd98f1d7bfb09937148fb1a57c6b52dd8"); \
    ADD_CHECKPOINT(75000, "b26f4e1225569da282b77659020bace52e5e89abbdee33e9e52266b1e71803a5"); \
    ADD_CHECKPOINT(100000, "ffe474fe8353f90700c8138ddea3547d5c1e4a6facb1df85897e7a6e4daab540"); \
    ADD_CHECKPOINT(116520, "da1cb8f30305cd5fad7d6c33b3ed88fede053e0926102d8e544b65e93e33a08b"); \
    ADD_CHECKPOINT(137500, "0a50041b952bdc1b1f2c6a5e8749600f545e43ddfa255607b529df95f8945e5d"); \
    ADD_CHECKPOINT(165000, "a15ab984e4c93bff84f617daaed357e28c5eb2fb6c64efa803f4cfba0b69f4a4"); \
    ADD_CHECKPOINT(199800, "d8c7fcfcf605e834b3125b68cc96736e1f1d2f753c79c24db8fb9d6af4b84293"); \
    ADD_CHECKPOINT(225000, "4a63e8c350e2ee31f2c35a290eee99c759cbadd4518dc9f13f24a8365ff62b43"); \
    ADD_CHECKPOINT(250000, "ea4ac947d9f0cdca5997871fb6a9e78f377bba17b5c70b87ce65693bc5032838"); \
    ADD_CHECKPOINT(274000, "49d2579161c277b9d9fe6baba5aabcef1534e9abef93eaa7f17cc8fe229454b0"); \
    ADD_CHECKPOINT(274360, "66c129116187f36980a97333f1c7cf99c21629cc52bc6d591126d3a8fe36b90a"); \
    ADD_CHECKPOINT(300000, "b09b147b23148d2995ff860d9ede9d8d38757c934b6de7945d397fc4e1ab2501"); \
    ADD_CHECKPOINT(320000, "3305f11f07669f156bbe9f9b07523d7b5a6f4c430475ae61c379c1e31984c1fb"); \
    ADD_CHECKPOINT(345000, "d67de29cf089207413686ceb385dbafcc9f7c12690e4a5d4ee9b42c274d2d4b3"); \
    ADD_CHECKPOINT(350000, "d3675de05fed5f6633288adb07ac4982b4c891be6347d9f6edfac9b21dbbd721"); \
    ADD_CHECKPOINT(375000, "405fdfcd94e6acecd0f282f41c0983c1b3437dce84e566f763f2fd683da02ea3"); \
    ADD_CHECKPOINT(394000, "86462c05c540b2df7ac39a8e5d85dc08d191b5cbc1ea59c8e5966bdd2bd28f31"); \
    ADD_CHECKPOINT(405000, "aaf7f8ceb403c3110dbb9256dd10f888524fed90159065400f52e663c1c3733f"); \
    ADD_CHECKPOINT(420000, "a67b28a7ec8785cdaf5d54cd56d5e92fe52dd2cbaa8ee7fab7eb27deb21b65ac"); \
    ADD_CHECKPOINT(435000, "ee11193a62f74d2ed681fd2e9212e9e4061774d9dd90039cc5a4d0b65f2c5522"); \
    ADD_CHECKPOINT(446000, "c50f8599b0c0cf5ad620217e9a496fdfa1f82b485995cfd73a04a1509bb902a2"); \
    ADD_CHECKPOINT(465000, "985a03585380e5a8ba30a2515174c05afbc71a858ec171a1c1a8658df322935e"); \

// testnet checkpoints
#define TESTNET_CHECKPOINTS \
    ADD_CHECKPOINT(1, "770309bd74bcebf90eb6f1197ad51601f917314088c5b1343316a4f581c51193"); \
    ADD_CHECKPOINT(10, "d5f139ed1cdc14ff2dda722de9ee2236f4030d18670e4ef6b41ab74e57a0a816"); \
    ADD_CHECKPOINT(100, "e88c3a0e2c0943c50ec9d7149994137dfe095201027920945038dbfc493e35eb"); \
    ADD_CHECKPOINT(1000, "5b33def41716acc581d891bb10ca3eb05867b16ac16764bf8732f12e2830b2a8"); \
    ADD_CHECKPOINT(10000, "71486869b52ed9f0e99a8d349e122bbe67f957e6b0a0b0361e566bd549b56afe"); \
    ADD_CHECKPOINT(30000, "1136333506ed1a4400cf5efaa318a11046a764924ca203a1d43865839eaee22c"); \
    ADD_CHECKPOINT(50000, "3d251e3802cc5f6721babf8762bc13fdbc57df694c59759f3baba8d28b7d97c0"); \
    ADD_CHECKPOINT(75000, "262c0a4cb5ba9651d12283edc3f9206ef12a93cdf2298e2b82f8b1b793eaee48"); \
    ADD_CHECKPOINT(100000, "5f3036ccbf91f825154d635b5355b202fe28fd17ef8253f4db93fb526641c78a"); \
    ADD_CHECKPOINT(125000, "a37fc6b3bcab2904a1a5bcbedc388844852b065eddafbb00e667a3340c04b2d4"); \
    ADD_CHECKPOINT(145000, "bb1dd8811b9c76c731c08c58cfec3b1e8f973e7a972566481c2f94e834a865f8"); \
    ADD_CHECKPOINT(164000, "adfb46976edc591fe9d0938a2cad4cbeed35cd79a8c75b65c7c420290e7d10a8"); \
    ADD_CHECKPOINT(190000, "2d42389a1fd0946c8084cd6141bf7562cb262093957f268208d068f9adbc9839"); \
    ADD_CHECKPOINT(210000, "1fd6d9669525a94cf5d034972d41a9ff4f57d28f1136bbdea395819d85dcc841"); \
    ADD_CHECKPOINT(225000, "8ee300b366f522f06869baf5c5cdbc7c23863fdf8f9d09be4c4d6618baffa62f"); \

// New constants are intended to go here
namespace config
{
  uint64_t const DEFAULT_FEE_ATOMIC_XMR_PER_KB = 500; // Just a placeholder!  Change me!
  uint8_t const FEE_CALCULATION_MAX_RETRIES = 10;
  uint64_t const DEFAULT_DUST_THRESHOLD = ((uint64_t)10000000); // pow(10, 7)
  uint64_t const BASE_REWARD_CLAMP_THRESHOLD = ((uint64_t)100000); // pow(10, 5)

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

  // Hash domain separators
  const char HASH_KEY_BULLETPROOF_EXPONENT[] = "bulletproof";
  const char HASH_KEY_RINGDB[] = "ringdsb";
  const char HASH_KEY_SUBADDRESS[] = "SubAddr";
  const unsigned char HASH_KEY_ENCRYPTED_PAYMENT_ID = 0x8d;
  const unsigned char HASH_KEY_WALLET = 0x8c;
  const unsigned char HASH_KEY_WALLET_CACHE = 0x8d;
  const unsigned char HASH_KEY_RPC_PAYMENT_NONCE = 0x58;
  const unsigned char HASH_KEY_MEMORY = 'k';
  const unsigned char HASH_KEY_MULTISIG[] = {'M', 'u', 'l', 't' , 'i', 's', 'i', 'g', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  const unsigned char HASH_KEY_TXPROOF_V2[] = "TXPROOF_V2";
  const unsigned char HASH_KEY_CLSAG_ROUND[] = "CLSAG_round";
  const unsigned char HASH_KEY_CLSAG_AGG_0[] = "CLSAG_agg_0";
  const unsigned char HASH_KEY_CLSAG_AGG_1[] = "CLSAG_agg_1";
  const char HASH_KEY_MESSAGE_SIGNING[] = "MoneroMessageSignature";

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
