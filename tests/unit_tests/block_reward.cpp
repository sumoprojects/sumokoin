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

#include "gtest/gtest.h"

#include "cryptonote_basic/cryptonote_basic_impl.h"

using namespace cryptonote;

namespace
{
  //--------------------------------------------------------------------------------------------------------------------
  class block_reward_and_already_generated_coins : public ::testing::Test
  {
  protected:
    static const size_t current_block_weight = CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE / 2;

    bool m_block_not_too_big;
    uint64_t m_block_reward;
  };

  #define TEST_ALREADY_GENERATED_COINS(already_generated_coins, expected_reward, height)                              \
    m_block_not_too_big = get_block_reward(0, current_block_weight, already_generated_coins, m_block_reward, height); \
    ASSERT_TRUE(m_block_not_too_big);                                                                         \
    ASSERT_EQ(m_block_reward, expected_reward);

  TEST_F(block_reward_and_already_generated_coins, handles_first_values)
  {
  	// 17592186044415 from neozaru, confirmed by fluffypony
    TEST_ALREADY_GENERATED_COINS(0, UINT64_C(32000000000), 100);
    TEST_ALREADY_GENERATED_COINS(m_block_reward, UINT64_C(32000000000), 100);
    TEST_ALREADY_GENERATED_COINS(UINT64_C(49), UINT64_C(48590000000), 197235);
  }

  //--------------------------------------------------------------------------------------------------------------------
  class block_reward_and_current_block_weight : public ::testing::Test
  {
  protected:
    virtual void SetUp()
    {
      m_block_not_too_big = get_block_reward(0, 0, already_generated_coins, m_standard_block_reward, 1);
      ASSERT_TRUE(m_block_not_too_big);
      ASSERT_LT(CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE, m_standard_block_reward);
    }

    void do_test(size_t median_block_weight, size_t current_block_weight)
    {
      m_block_not_too_big = get_block_reward(median_block_weight, current_block_weight, already_generated_coins, m_block_reward, 1);
    }

    static const uint64_t already_generated_coins = 0;

    bool m_block_not_too_big;
    uint64_t m_block_reward;
    uint64_t m_standard_block_reward;
  };

  TEST_F(block_reward_and_current_block_weight, handles_block_weight_less_relevance_level)
  {
    do_test(0, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE - 1);
    ASSERT_TRUE(m_block_not_too_big);
    ASSERT_EQ(m_block_reward, m_standard_block_reward);
  }

  TEST_F(block_reward_and_current_block_weight, handles_block_weight_eq_relevance_level)
  {
    do_test(0, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE);
    ASSERT_TRUE(m_block_not_too_big);
    ASSERT_EQ(m_block_reward, m_standard_block_reward);
  }

  TEST_F(block_reward_and_current_block_weight, handles_block_weight_gt_relevance_level)
  {
    do_test(0, ((11 * CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE)/10) + 1);
    ASSERT_TRUE(m_block_not_too_big);
    ASSERT_LT(m_block_reward, m_standard_block_reward);
  }

  TEST_F(block_reward_and_current_block_weight, handles_block_weight_less_2_relevance_level)
  {
    do_test(0, 2 * CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE - 1);
    ASSERT_TRUE(m_block_not_too_big);
    ASSERT_LT(m_block_reward, m_standard_block_reward);
    ASSERT_LT(0, m_block_reward);
  }

  TEST_F(block_reward_and_current_block_weight, handles_block_weight_eq_2_relevance_level)
  {
    do_test(0, 2 * CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE);
    ASSERT_TRUE(m_block_not_too_big);
    ASSERT_EQ(0, m_block_reward);
  }

  TEST_F(block_reward_and_current_block_weight, handles_block_weight_gt_2_relevance_level)
  {
    do_test(0, 2 * CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE + 1);
    ASSERT_FALSE(m_block_not_too_big);
  }

#ifdef __x86_64__ // For 64-bit systems only, because block size is limited to size_t.
  TEST_F(block_reward_and_current_block_weight, fails_on_huge_median_size)
  {
#if !defined(NDEBUG)
    size_t huge_size = std::numeric_limits<uint32_t>::max() + UINT64_C(2);
    ASSERT_DEATH(do_test(huge_size, huge_size + 1), "");
#endif
  }

  TEST_F(block_reward_and_current_block_weight, fails_on_huge_block_weight)
  {
#if !defined(NDEBUG)
    size_t huge_size = std::numeric_limits<uint32_t>::max() + UINT64_C(2);
    ASSERT_DEATH(do_test(huge_size - 2, huge_size), "");
#endif
  }
#endif // __x86_64__

  //--------------------------------------------------------------------------------------------------------------------
  class block_reward_and_last_block_weights : public ::testing::Test
  {
  protected:
    virtual void SetUp()
    {
      m_last_block_weights.push_back(3  * CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE);
      m_last_block_weights.push_back(5  * CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE);
      m_last_block_weights.push_back(7  * CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE);
      m_last_block_weights.push_back(11 * CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE);
      m_last_block_weights.push_back(13 * CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE);

      m_last_block_weights_median = 7 * CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE;

      m_block_not_too_big = get_block_reward(epee::misc_utils::median(m_last_block_weights), 0, already_generated_coins, m_standard_block_reward, 1);
      ASSERT_TRUE(m_block_not_too_big);
      ASSERT_LT(CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE, m_standard_block_reward);
    }

    void do_test(size_t current_block_weight)
    {
      m_block_not_too_big = get_block_reward(epee::misc_utils::median(m_last_block_weights), current_block_weight, already_generated_coins, m_block_reward, 1);
    }

    static const uint64_t already_generated_coins = 0;

    std::vector<size_t> m_last_block_weights;
    uint64_t m_last_block_weights_median;
    bool m_block_not_too_big;
    uint64_t m_block_reward;
    uint64_t m_standard_block_reward;
  };

  TEST_F(block_reward_and_last_block_weights, handles_block_weight_less_median)
  {
    do_test(m_last_block_weights_median - 1);
    ASSERT_TRUE(m_block_not_too_big);
    ASSERT_EQ(m_block_reward, m_standard_block_reward);
  }

  TEST_F(block_reward_and_last_block_weights, handles_block_weight_eq_median)
  {
    do_test(m_last_block_weights_median);
    ASSERT_TRUE(m_block_not_too_big);
    ASSERT_EQ(m_block_reward, m_standard_block_reward);
  }

  TEST_F(block_reward_and_last_block_weights, handles_block_weight_gt_median)
  {
    do_test(m_last_block_weights_median + 1);
    ASSERT_TRUE(m_block_not_too_big);
    ASSERT_LT(m_block_reward, m_standard_block_reward);
  }

  TEST_F(block_reward_and_last_block_weights, handles_block_weight_less_2_medians)
  {
    do_test(2 * m_last_block_weights_median - 1);
    ASSERT_TRUE(m_block_not_too_big);
    ASSERT_LT(m_block_reward, m_standard_block_reward);
    ASSERT_LT(0, m_block_reward);
  }

  TEST_F(block_reward_and_last_block_weights, handles_block_weight_eq_2_medians)
  {
    do_test(2 * m_last_block_weights_median);
    ASSERT_TRUE(m_block_not_too_big);
    ASSERT_EQ(0, m_block_reward);
  }

  TEST_F(block_reward_and_last_block_weights, handles_block_weight_gt_2_medians)
  {
    do_test(2 * m_last_block_weights_median + 1);
    ASSERT_FALSE(m_block_not_too_big);
  }

  TEST_F(block_reward_and_last_block_weights, calculates_correctly)
  {
    ASSERT_EQ(0, m_last_block_weights_median % 8);

    do_test(m_last_block_weights_median * 9 / 8);
    ASSERT_TRUE(m_block_not_too_big);
    ASSERT_EQ(m_block_reward, m_standard_block_reward * 63 / 64);

    // 3/2 = 12/8
    do_test(m_last_block_weights_median * 3 / 2);
    ASSERT_TRUE(m_block_not_too_big);
    ASSERT_EQ(m_block_reward, m_standard_block_reward * 3 / 4);

    do_test(m_last_block_weights_median * 15 / 8);
    ASSERT_TRUE(m_block_not_too_big);
    ASSERT_EQ(m_block_reward, m_standard_block_reward * 15 / 64);
  }
}
