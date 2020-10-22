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

namespace config
{
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
}
