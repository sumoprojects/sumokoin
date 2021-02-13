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
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "checkpoints.h"

#include "common/dns_utils.h"
#include "string_tools.h"
#include "storages/portable_storage_template_helper.h" // epee json include
#include "serialization/keyvalue_serialization.h"
#include <vector>

using namespace epee;

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "checkpoints"

namespace cryptonote
{
  /**
   * @brief struct for loading a checkpoint from json
   */
  struct t_hashline
  {
    uint64_t height; //!< the height of the checkpoint
    std::string hash; //!< the hash for the checkpoint
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(height)
          KV_SERIALIZE(hash)
        END_KV_SERIALIZE_MAP()
  };

  /**
   * @brief struct for loading many checkpoints from json
   */
  struct t_hash_json {
    std::vector<t_hashline> hashlines; //!< the checkpoint lines from the file
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(hashlines)
        END_KV_SERIALIZE_MAP()
  };

  //---------------------------------------------------------------------------
  checkpoints::checkpoints()
  {
  }
  //---------------------------------------------------------------------------
  bool checkpoints::add_checkpoint(uint64_t height, const std::string& hash_str)
  {
    crypto::hash h = crypto::null_hash;
    bool r = epee::string_tools::hex_to_pod(hash_str, h);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse checkpoint hash string into binary representation!");

    // return false if adding at a height we already have AND the hash is different
    if (m_points.count(height))
    {
      CHECK_AND_ASSERT_MES(h == m_points[height], false, "Checkpoint at given height already exists, and hash for new checkpoint was different!");
    }
    m_points[height] = h;
    return true;
  }
  //---------------------------------------------------------------------------
  bool checkpoints::is_in_checkpoint_zone(uint64_t height) const
  {
    return !m_points.empty() && (height <= (--m_points.end())->first);
  }
  //---------------------------------------------------------------------------
  bool checkpoints::check_block(uint64_t height, const crypto::hash& h, bool& is_a_checkpoint) const
  {
    auto it = m_points.find(height);
    is_a_checkpoint = it != m_points.end();
    if(!is_a_checkpoint)
      return true;

    if(it->second == h)
    {
      MINFO("CHECKPOINT PASSED FOR HEIGHT " << height << " " << h);
      return true;
    }else
    {
      MWARNING("CHECKPOINT FAILED FOR HEIGHT " << height << ". EXPECTED HASH: " << it->second << ", FETCHED HASH: " << h);
      return false;
    }
  }
  //---------------------------------------------------------------------------
  bool checkpoints::check_block(uint64_t height, const crypto::hash& h) const
  {
    bool ignored;
    return check_block(height, h, ignored);
  }
  //---------------------------------------------------------------------------
  //FIXME: is this the desired behavior?
  bool checkpoints::is_alternative_block_allowed(uint64_t blockchain_height, uint64_t block_height) const
  {
    if (0 == block_height)
      return false;

    auto it = m_points.upper_bound(blockchain_height);
    // Is blockchain_height before the first checkpoint?
    if (it == m_points.begin())
      return true;

    --it;
    uint64_t checkpoint_height = it->first;
    return checkpoint_height < block_height;
  }
  //---------------------------------------------------------------------------
  uint64_t checkpoints::get_max_height() const
  {
    std::map< uint64_t, crypto::hash >::const_iterator highest =
        std::max_element( m_points.begin(), m_points.end(),
                         ( boost::bind(&std::map< uint64_t, crypto::hash >::value_type::first, _1) <
                           boost::bind(&std::map< uint64_t, crypto::hash >::value_type::first, _2 ) ) );
    return highest->first;
  }
  //---------------------------------------------------------------------------
  const std::map<uint64_t, crypto::hash>& checkpoints::get_points() const
  {
    return m_points;
  }

  bool checkpoints::check_for_conflicts(const checkpoints& other) const
  {
    for (auto& pt : other.get_points())
    {
      if (m_points.count(pt.first))
      {
        CHECK_AND_ASSERT_MES(pt.second == m_points.at(pt.first), false, "Checkpoint at given height already exists, and hash for new checkpoint was different!");
      }
    }
    return true;
  }

  bool checkpoints::init_default_checkpoints(network_type nettype)
  {
    if (nettype == TESTNET)
    {
      return true;
    }
    if (nettype == STAGENET)
    {
      return true;
    }

    ADD_CHECKPOINT(1, "82e8f378ea29d152146b6317903249751b809e97c0b6655f86e120b9de95c38a");
    ADD_CHECKPOINT(10, "e097b62bba41e5fd583d3a68de074cddd77c85a6158b031d963232146494a2d6");
    ADD_CHECKPOINT(100, "f3bd44c626cc12d449183ca84b58615d792523ba229385ff6717ab29a3e88926");
    ADD_CHECKPOINT(1000, "d284c992cb570f86c2e0bcfaa552b1d73bd40417e1c2a40f82bc6432217f0873");
    ADD_CHECKPOINT(3000, "81e040955b710dc5a5056668c4eaf3fbc4da2f72c0a63763250ede32a92e0f06");
    ADD_CHECKPOINT(5000, "e838c077bc66356d9bb321d4eb60f0851ef766f0619ddc4c6568a0f149aacea0");
    ADD_CHECKPOINT(10000, "360b96c3d0a5202c548672d550700d982ca15ad5627f70bce0a89dda840b3611");
    ADD_CHECKPOINT(20000, "603a45b60dd92ef4524c80d58411d09480b4668c54bc08dd651d838832bd399e");
    ADD_CHECKPOINT(21300, "d0a76e98ebb4d8e928d931a1807bba11a2fafdf544c119761b0ed6de4e1898cf"); // v2 fork
    ADD_CHECKPOINT(50000, "ae36641cf06ed788375bfb32f0038cbcd98f1d7bfb09937148fb1a57c6b52dd8");
    ADD_CHECKPOINT(75000, "b26f4e1225569da282b77659020bace52e5e89abbdee33e9e52266b1e71803a5");
    ADD_CHECKPOINT(100000, "ffe474fe8353f90700c8138ddea3547d5c1e4a6facb1df85897e7a6e4daab540");
    ADD_CHECKPOINT(116520, "da1cb8f30305cd5fad7d6c33b3ed88fede053e0926102d8e544b65e93e33a08b"); // v3 fork
    ADD_CHECKPOINT(137500, "0a50041b952bdc1b1f2c6a5e8749600f545e43ddfa255607b529df95f8945e5d"); // v4 fork
    ADD_CHECKPOINT(165000, "a15ab984e4c93bff84f617daaed357e28c5eb2fb6c64efa803f4cfba0b69f4a4"); // v5 fork
    ADD_CHECKPOINT(199800, "d8c7fcfcf605e834b3125b68cc96736e1f1d2f753c79c24db8fb9d6af4b84293"); // v6 fork
    ADD_CHECKPOINT(274000, "49d2579161c277b9d9fe6baba5aabcef1534e9abef93eaa7f17cc8fe229454b0"); // v7 fork
    ADD_CHECKPOINT(274360, "66c129116187f36980a97333f1c7cf99c21629cc52bc6d591126d3a8fe36b90a"); // v8 fork
    ADD_CHECKPOINT(300000, "b09b147b23148d2995ff860d9ede9d8d38757c934b6de7945d397fc4e1ab2501");
    ADD_CHECKPOINT(320000, "3305f11f07669f156bbe9f9b07523d7b5a6f4c430475ae61c379c1e31984c1fb");
    ADD_CHECKPOINT(345000, "d67de29cf089207413686ceb385dbafcc9f7c12690e4a5d4ee9b42c274d2d4b3");
    ADD_CHECKPOINT(350000, "d3675de05fed5f6633288adb07ac4982b4c891be6347d9f6edfac9b21dbbd721"); // v9 fork
    ADD_CHECKPOINT(375000, "405fdfcd94e6acecd0f282f41c0983c1b3437dce84e566f763f2fd683da02ea3");
    ADD_CHECKPOINT(394000, "86462c05c540b2df7ac39a8e5d85dc08d191b5cbc1ea59c8e5966bdd2bd28f31");
    ADD_CHECKPOINT(405000, "aaf7f8ceb403c3110dbb9256dd10f888524fed90159065400f52e663c1c3733f");
    ADD_CHECKPOINT(420000, "a67b28a7ec8785cdaf5d54cd56d5e92fe52dd2cbaa8ee7fab7eb27deb21b65ac");
    ADD_CHECKPOINT(435000, "ee11193a62f74d2ed681fd2e9212e9e4061774d9dd90039cc5a4d0b65f2c5522");
    ADD_CHECKPOINT(446000, "c50f8599b0c0cf5ad620217e9a496fdfa1f82b485995cfd73a04a1509bb902a2");
    ADD_CHECKPOINT(465000, "985a03585380e5a8ba30a2515174c05afbc71a858ec171a1c1a8658df322935e"); 
    ADD_CHECKPOINT(492000, "a2efbec083be4f1aadb1e368e85fc861a3014bcef6e58dad2161b46cc6fa0cea");
    return true;
  }

  bool checkpoints::load_checkpoints_from_json(const std::string &json_hashfile_fullpath)
  {
    boost::system::error_code errcode;
    if (! (boost::filesystem::exists(json_hashfile_fullpath, errcode)))
    {
      LOG_PRINT_L1("Blockchain checkpoints file not found");
      return true;
    }

    LOG_PRINT_L1("Adding checkpoints from blockchain hashfile");

    uint64_t prev_max_height = get_max_height();
    LOG_PRINT_L1("Hard-coded max checkpoint height is " << prev_max_height);
    t_hash_json hashes;
    if (!epee::serialization::load_t_from_json_file(hashes, json_hashfile_fullpath))
    {
      MERROR("Error loading checkpoints from " << json_hashfile_fullpath);
      return false;
    }
    for (std::vector<t_hashline>::const_iterator it = hashes.hashlines.begin(); it != hashes.hashlines.end(); )
    {
      uint64_t height;
      height = it->height;
      if (height <= prev_max_height) {
	LOG_PRINT_L1("ignoring checkpoint height " << height);
      } else {
	std::string blockhash = it->hash;
	LOG_PRINT_L1("Adding checkpoint height " << height << ", hash=" << blockhash);
	ADD_CHECKPOINT(height, blockhash);
      }
      ++it;
    }

    return true;
  }

  bool checkpoints::load_checkpoints_from_dns(network_type nettype)
  {
    std::vector<std::string> records;

    // All four SumoPulse domains have DNSSEC on and valid
    static const std::vector<std::string> dns_urls = { "checkpoints.sumopulse.stream"
                   , "checkpoints.sumopulse.download"
                   , "checkpoints.sumopulse.win"
                   , "checkpoints.sumopulse.bid"
    };

    static const std::vector<std::string> testnet_dns_urls = { "testpoints.sumopulse.stream"
                   , "testpoints.sumopulse.download"
                   , "testpoints.sumopulse.win"
                   , "testpoints.sumopulse.bid"
    };

    static const std::vector<std::string> stagenet_dns_urls = { "stagenetpoints.sumopulse.stream"
                   , "stagenetpoints.sumopulse.download"
                   , "stagenetpoints.sumopulse.win"
                   , "stagenetpoints.sumopulse.bid"
    };

    if (!tools::dns_utils::load_txt_records_from_dns(records, nettype == TESTNET ? testnet_dns_urls : nettype == STAGENET ? stagenet_dns_urls : dns_urls))
      return true; // why true ?

    for (const auto& record : records)
    {
      auto pos = record.find(":");
      if (pos != std::string::npos)
      {
        uint64_t height;
        crypto::hash hash;

        // parse the first part as uint64_t,
        // if this fails move on to the next record
        std::stringstream ss(record.substr(0, pos));
        if (!(ss >> height))
        {
    continue;
        }

        // parse the second part as crypto::hash,
        // if this fails move on to the next record
        std::string hashStr = record.substr(pos + 1);
        if (!epee::string_tools::hex_to_pod(hashStr, hash))
        {
    continue;
        }

        ADD_CHECKPOINT(height, hashStr);
      }
    }
    return true;
  }

  bool checkpoints::load_new_checkpoints(const std::string &json_hashfile_fullpath, network_type nettype, bool dns)
  {
    bool result;

    result = load_checkpoints_from_json(json_hashfile_fullpath);
    if (dns)
    {
      result &= load_checkpoints_from_dns(nettype);
    }

    return result;
  }
}
