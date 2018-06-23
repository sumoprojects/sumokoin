// Copyright (c) 2017, SUMOKOIN
// Copyright (c) 2014-2016, The Monero Project
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

#include "include_base_utils.h"

using namespace epee;

#include "checkpoints.h"

#include "common/dns_utils.h"
#include "include_base_utils.h"
#include <sstream>
#include <random>

namespace
{
  bool dns_records_match(const std::vector<std::string>& a, const std::vector<std::string>& b)
  {
    if (a.size() != b.size()) return false;

    for (const auto& record_in_a : a)
    {
      bool ok = false;
      for (const auto& record_in_b : b)
      {
	if (record_in_a == record_in_b)
	{
	  ok = true;
	  break;
	}
      }
      if (!ok) return false;
    }

    return true;
  }
} // anonymous namespace

namespace cryptonote
{
  //---------------------------------------------------------------------------
  checkpoints::checkpoints()
  {
  }
  //---------------------------------------------------------------------------
  bool checkpoints::add_checkpoint(uint64_t height, const std::string& hash_str)
  {
    crypto::hash h = null_hash;
    bool r = epee::string_tools::parse_tpod_from_hex_string(hash_str, h);
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
      LOG_PRINT_GREEN("CHECKPOINT PASSED FOR HEIGHT " << height << " " << h, LOG_LEVEL_1);
      return true;
    }else
    {
      LOG_ERROR("CHECKPOINT FAILED FOR HEIGHT " << height << ". EXPECTED HASH: " << it->second << ", FETCHED HASH: " << h);
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

  bool checkpoints::init_default_checkpoints()
  {
    ADD_CHECKPOINT(1,     "82e8f378ea29d152146b6317903249751b809e97c0b6655f86e120b9de95c38a");
    ADD_CHECKPOINT(10,    "e097b62bba41e5fd583d3a68de074cddd77c85a6158b031d963232146494a2d6");
    ADD_CHECKPOINT(100,   "f3bd44c626cc12d449183ca84b58615d792523ba229385ff6717ab29a3e88926");
    ADD_CHECKPOINT(1000,  "d284c992cb570f86c2e0bcfaa552b1d73bd40417e1c2a40f82bc6432217f0873");
    ADD_CHECKPOINT(3000,  "81e040955b710dc5a5056668c4eaf3fbc4da2f72c0a63763250ede32a92e0f06");
    ADD_CHECKPOINT(5000,  "e838c077bc66356d9bb321d4eb60f0851ef766f0619ddc4c6568a0f149aacea0");
    ADD_CHECKPOINT(10000, "360b96c3d0a5202c548672d550700d982ca15ad5627f70bce0a89dda840b3611");
    ADD_CHECKPOINT(20000, "603a45b60dd92ef4524c80d58411d09480b4668c54bc08dd651d838832bd399e");
    ADD_CHECKPOINT(21300, "d0a76e98ebb4d8e928d931a1807bba11a2fafdf544c119761b0ed6de4e1898cf"); // v2 fork
    ADD_CHECKPOINT(50000, "ae36641cf06ed788375bfb32f0038cbcd98f1d7bfb09937148fb1a57c6b52dd8");
    ADD_CHECKPOINT(75000, "b26f4e1225569da282b77659020bace52e5e89abbdee33e9e52266b1e71803a5");
    ADD_CHECKPOINT(100000, "ffe474fe8353f90700c8138ddea3547d5c1e4a6facb1df85897e7a6e4daab540");
    ADD_CHECKPOINT(116520, "da1cb8f30305cd5fad7d6c33b3ed88fede053e0926102d8e544b65e93e33a08b"); // v3 fork
    ADD_CHECKPOINT(137500, "0a50041b952bdc1b1f2c6a5e8749600f545e43ddfa255607b529df95f8945e5d"); // v4 fork
    return true;
  }

  bool checkpoints::load_checkpoints_from_json(const std::string json_hashfile_fullpath)
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
    epee::serialization::load_t_from_json_file(hashes, json_hashfile_fullpath);
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

  bool checkpoints::load_checkpoints_from_dns(bool testnet)
  {
    // All SumoPulse domains have DNSSEC on and valid
    static const std::vector<std::string> dns_urls = 
    {
	"checkpoints.sumopulse.stream"
	, "checkpoints.sumopulse.download"
	, "checkpoints.sumopulse.win"
	, "checkpoints.sumopulse.bid"
    };

    static const std::vector<std::string> testnet_dns_urls = { 
	"testpoints.sumopulse.stream"
	, "testpoints.sumopulse.download"
	, "testpoints.sumopulse.win"
	, "testpoints.sumopulse.bid"
    };

    std::vector<std::vector<std::string> > records;
    records.resize(dns_urls.size());

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, dns_urls.size() - 1);
    size_t first_index = dis(gen);

    bool avail, valid;
    size_t cur_index = first_index;
    do
    {
      std::string url;
      if (testnet)
      {
        url = testnet_dns_urls[cur_index];
      }
      else
      {
        url = dns_urls[cur_index];
      }

      records[cur_index] = tools::DNSResolver::instance().get_txt_record(url, avail, valid);
      if (!avail)
      {
        records[cur_index].clear();
        LOG_PRINT_L2("DNSSEC not available for checkpoint update at URL: " << url << ", skipping.");
      }
      if (!valid)
      {
        records[cur_index].clear();
        LOG_PRINT_L2("DNSSEC validation failed for checkpoint update at URL: " << url << ", skipping.");
      }

      cur_index++;
      if (cur_index == dns_urls.size())
      {
        cur_index = 0;
      }
      records[cur_index].clear();
    } while (cur_index != first_index);

    size_t num_valid_records = 0;

    for( const auto& record_set : records)
    {
      if (record_set.size() != 0)
      {
        num_valid_records++;
      }
    }

    if (num_valid_records < 2)
    {
      LOG_PRINT_L0("WARNING: no two valid SumoPulse DNS checkpoint records were received");
      return true;
    }

    int good_records_index = -1;
    for (size_t i = 0; i < records.size() - 1; ++i)
    {
      if (records[i].size() == 0) continue;

      for (size_t j = i + 1; j < records.size(); ++j)
      {
        if (dns_records_match(records[i], records[j]))
        {
    good_records_index = i;
    break;
        }
      }
      if (good_records_index >= 0) break;
    }

    if (good_records_index < 0)
    {
      LOG_PRINT_L0("WARNING: no two SumoPulse DNS checkpoint records matched");
      return true;
    }

    for (auto& record : records[good_records_index])
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
        if (!epee::string_tools::parse_tpod_from_hex_string(hashStr, hash))
        {
    continue;
        }

        ADD_CHECKPOINT(height, hashStr);
      }
    }
    return true;
  }

  bool checkpoints::load_new_checkpoints(const std::string json_hashfile_fullpath, bool testnet, bool dns)
  {
    bool result;

    result = load_checkpoints_from_json(json_hashfile_fullpath);
    if (dns)
    {
      result &= load_checkpoints_from_dns(testnet);
    }

    return result;
  }
}
