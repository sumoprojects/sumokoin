// Copyright (c) 2017-2019, Sumokoin Project
// Copyright (c) 2014-2019, The Monero Project
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

#include "hardforks.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "blockchain.hardforks"

const hardfork_t mainnet_hard_forks[] = {
  { 1, 1, 0, 1482806500 },
  { 2, 21300, 0, 1497657600 },
  { 3, MAINNET_HARDFORK_V3_HEIGHT, 0, 1522800000 },
  { 4, 137500, 0, 1528045200 },
  { 5, 165000, 0, 1529643600 },
  { 6, MAINNET_HARDFORK_V6_HEIGHT, 0, 1537065522 },
  { 7, MAINNET_HARDFORK_V7_HEIGHT, 0, 1555234940 },
  { 8, MAINNET_HARDFORK_V8_HEIGHT, 0, 1555321375 },
  { 9, 350000, 0, 1574120819 }, // abt 6h47' Nov 19, 2019
};
const size_t num_mainnet_hard_forks = sizeof(mainnet_hard_forks) / sizeof(mainnet_hard_forks[0]);
const uint64_t mainnet_hard_fork_version_1_till = (uint64_t)-1;

const hardfork_t testnet_hard_forks[] = {
  { 1, 1, 0, 1482806500 },
  { 2, 5150, 0, 1497181713 },
  { 3, 103580, 0, 1522540800 },
  { 4, 122452, 0, 1527699600 },
  { 5, 128680, 0, 1529308166 },
  { 6, 130500, 0, 1554265083 },
  { 7, 130530, 0, 1554465078 },
  { 8, 130560, 0, 1554479506 },
  { 9, 164100, 0, 1572592223 },
};
const size_t num_testnet_hard_forks = sizeof(testnet_hard_forks) / sizeof(testnet_hard_forks[0]);
const uint64_t testnet_hard_fork_version_1_till = (uint64_t)-1;;

const hardfork_t stagenet_hard_forks[] = {
  { 1, 1, 0, 1482806500 },
  { 2, 5150, 0, 1497181713 },
  { 3, 103580, 0, 1522540800 },
  { 4, 122452, 0, 1527699600 },
  { 5, 128680, 0, 1529308166 },
  { 6, 130500, 0, 1554265083 },
  { 7, 130530, 0, 1554465078 },
  { 8, 130560, 0, 1554479506 },
  { 9, 164100, 0, 1572592223 },
};
const size_t num_stagenet_hard_forks = sizeof(stagenet_hard_forks) / sizeof(stagenet_hard_forks[0]);
