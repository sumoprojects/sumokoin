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

#include <boost/range/adaptor/reversed.hpp>

#include "string_tools.h"
#include "blockchain_db.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "profile_tools.h"
#include "ringct/rctOps.h"

#include "lmdb/db_lmdb.h"
#ifdef BERKELEY_DB
#include "berkeleydb/db_bdb.h"
#endif

static const char *db_types[] = {
  "lmdb",
#ifdef BERKELEY_DB
  "berkeley",
#endif
  NULL
};

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "blockchain.db"

using epee::string_tools::pod_to_hex;

namespace cryptonote
{

bool blockchain_valid_db_type(const std::string& db_type)
{
  int i;
  for (i=0; db_types[i]; i++)
  {
    if (db_types[i] == db_type)
      return true;
  }
  return false;
}

std::string blockchain_db_types(const std::string& sep)
{
  int i;
  std::string ret = "";
  for (i=0; db_types[i]; i++)
  {
    if (i)
      ret += sep;
    ret += db_types[i];
  }
  return ret;
}

std::string arg_db_type_description = "Specify database type, available: " + cryptonote::blockchain_db_types(", ");
const command_line::arg_descriptor<std::string> arg_db_type = {
  "db-type"
, arg_db_type_description.c_str()
, DEFAULT_DB_TYPE
};
const command_line::arg_descriptor<std::string> arg_db_sync_mode = {
  "db-sync-mode"
, "Specify sync option, using format [safe|fast|fastest]:[sync|async]:[nblocks_per_sync]." 
, "fast:async:1000"
};
const command_line::arg_descriptor<bool> arg_db_salvage  = {
  "db-salvage"
, "Try to salvage a blockchain database if it seems corrupted"
, false
};

BlockchainDB *new_db(const std::string& db_type)
{
  if (db_type == "lmdb")
    return new BlockchainLMDB();
#if defined(BERKELEY_DB)
  if (db_type == "berkeley")
    return new BlockchainBDB();
#endif
  return NULL;
}

void BlockchainDB::init_options(boost::program_options::options_description& desc)
{
  command_line::add_arg(desc, arg_db_type);
  command_line::add_arg(desc, arg_db_sync_mode);
  command_line::add_arg(desc, arg_db_salvage);
}

void BlockchainDB::pop_block()
{
  block blk;
  std::vector<transaction> txs;
  pop_block(blk, txs);
}

void BlockchainDB::add_transaction(const crypto::hash& blk_hash, const transaction& tx, const crypto::hash* tx_hash_ptr)
{
  bool miner_tx = false;
  crypto::hash tx_hash;
  if (!tx_hash_ptr)
  {
    // should only need to compute hash for miner transactions
    tx_hash = get_transaction_hash(tx);
    LOG_PRINT_L3("null tx_hash_ptr - needed to compute: " << tx_hash);
  }
  else
  {
    tx_hash = *tx_hash_ptr;
  }

  for (const txin_v& tx_input : tx.vin)
  {
    if (tx_input.type() == typeid(txin_to_key))
    {
      add_spent_key(boost::get<txin_to_key>(tx_input).k_image);
    }
    else if (tx_input.type() == typeid(txin_gen))
    {
      /* nothing to do here */
      miner_tx = true;
    }
    else
    {
      LOG_PRINT_L1("Unsupported input type, removing key images and aborting transaction addition");
      for (const txin_v& tx_input : tx.vin)
      {
        if (tx_input.type() == typeid(txin_to_key))
        {
          remove_spent_key(boost::get<txin_to_key>(tx_input).k_image);
        }
      }
      return;
    }
  }

  uint64_t tx_id = add_transaction_data(blk_hash, tx, tx_hash);

  std::vector<uint64_t> amount_output_indices;

  // iterate tx.vout using indices instead of C++11 foreach syntax because
  // we need the index
  for (uint64_t i = 0; i < tx.vout.size(); ++i)
  {
    // miner v2 txes have their coinbase output in one single out to save space,
    // and we store them as rct outputs with an identity mask
    if (miner_tx && tx.version == 2)
    {
      cryptonote::tx_out vout = tx.vout[i];
      rct::key commitment = rct::zeroCommit(vout.amount);
      vout.amount = 0;
      amount_output_indices.push_back(add_output(tx_hash, vout, i, tx.unlock_time,
        &commitment));
    }
    else
    {
      amount_output_indices.push_back(add_output(tx_hash, tx.vout[i], i, tx.unlock_time,
        tx.version > 1 ? &tx.rct_signatures.outPk[i].mask : NULL));
    }
  }
  add_tx_amount_output_indices(tx_id, amount_output_indices);
}

uint64_t BlockchainDB::add_block( const block& blk
                                , const size_t& block_size
                                , const difficulty_type& cumulative_difficulty
                                , const uint64_t& coins_generated
                                , const std::vector<transaction>& txs
                                )
{
  // sanity
  if (blk.tx_hashes.size() != txs.size())
    throw std::runtime_error("Inconsistent tx/hashes sizes");

  block_txn_start(false);

  TIME_MEASURE_START(time1);
  crypto::hash blk_hash = get_block_hash(blk);
  TIME_MEASURE_FINISH(time1);
  time_blk_hash += time1;

  uint64_t prev_height = height();

  // call out to add the transactions

  time1 = epee::misc_utils::get_tick_count();
  add_transaction(blk_hash, blk.miner_tx);
  int tx_i = 0;
  crypto::hash tx_hash = crypto::null_hash;
  for (const transaction& tx : txs)
  {
    tx_hash = blk.tx_hashes[tx_i];
    add_transaction(blk_hash, tx, &tx_hash);
    ++tx_i;
  }
  TIME_MEASURE_FINISH(time1);
  time_add_transaction += time1;

  // call out to subclass implementation to add the block & metadata
  time1 = epee::misc_utils::get_tick_count();
  add_block(blk, block_size, cumulative_difficulty, coins_generated, blk_hash);
  TIME_MEASURE_FINISH(time1);
  time_add_block1 += time1;

  m_hardfork->add(blk, prev_height);

  block_txn_stop();

  ++num_calls;

  return prev_height;
}

void BlockchainDB::set_hard_fork(HardFork* hf)
{
  m_hardfork = hf;
}

void BlockchainDB::pop_block(block& blk, std::vector<transaction>& txs)
{
  blk = get_top_block();

  remove_block();

  for (const auto& h : boost::adaptors::reverse(blk.tx_hashes))
  {
    txs.push_back(get_tx(h));
    remove_transaction(h);
  }
  remove_transaction(get_transaction_hash(blk.miner_tx));
}

bool BlockchainDB::is_open() const
{
  return m_open;
}

void BlockchainDB::remove_transaction(const crypto::hash& tx_hash)
{
  transaction tx = get_tx(tx_hash);

  for (const txin_v& tx_input : tx.vin)
  {
    if (tx_input.type() == typeid(txin_to_key))
    {
      remove_spent_key(boost::get<txin_to_key>(tx_input).k_image);
    }
  }

  // need tx as tx.vout has the tx outputs, and the output amounts are needed
  remove_transaction_data(tx_hash, tx);
}

block BlockchainDB::get_block_from_height(const uint64_t& height) const
{
  blobdata bd = get_block_blob_from_height(height);
  block b;
  if (!parse_and_validate_block_from_blob(bd, b))
    throw DB_ERROR("Failed to parse block from blob retrieved from the db");

  return b;
}

block BlockchainDB::get_block(const crypto::hash& h) const
{
  blobdata bd = get_block_blob(h);
  block b;
  if (!parse_and_validate_block_from_blob(bd, b))
    throw DB_ERROR("Failed to parse block from blob retrieved from the db");

  return b;
}

bool BlockchainDB::get_tx(const crypto::hash& h, cryptonote::transaction &tx) const
{
  blobdata bd;
  if (!get_tx_blob(h, bd))
    return false;
  if (!parse_and_validate_tx_from_blob(bd, tx))
    throw DB_ERROR("Failed to parse transaction from blob retrieved from the db");

  return true;
}

transaction BlockchainDB::get_tx(const crypto::hash& h) const
{
  transaction tx;
  if (!get_tx(h, tx))
    throw TX_DNE(std::string("tx with hash ").append(epee::string_tools::pod_to_hex(h)).append(" not found in db").c_str());
  return tx;
}

void BlockchainDB::reset_stats()
{
  num_calls = 0;
  time_blk_hash = 0;
  time_tx_exists = 0;
  time_add_block1 = 0;
  time_add_transaction = 0;
  time_commit1 = 0;
}

void BlockchainDB::show_stats()
{
  LOG_PRINT_L1(ENDL
    << "*********************************"
    << ENDL
    << "num_calls: " << num_calls
    << ENDL
    << "time_blk_hash: " << time_blk_hash << "ms"
    << ENDL
    << "time_tx_exists: " << time_tx_exists << "ms"
    << ENDL
    << "time_add_block1: " << time_add_block1 << "ms"
    << ENDL
    << "time_add_transaction: " << time_add_transaction << "ms"
    << ENDL
    << "time_commit1: " << time_commit1 << "ms"
    << ENDL
    << "*********************************"
    << ENDL
  );
}

void BlockchainDB::fixup()
{
  if (is_read_only()) {
    LOG_PRINT_L1("Database is opened read only - skipping fixup check");
    return;
  }

  // There was a bug that would cause key images for transactions without
  // any outputs to not be added to the spent key image set. There are two
  // instances of such transactions, in blocks 202612 and 685498.
  // The key images below are those from the inputs in those transactions.
  // On testnet, there are no such transactions
  // See commit 533acc30eda7792c802ea8b6417917fa99b8bc2b for the fix
  static const char * const mainnet_genesis_hex = "6eb04b6b8c68049a76206fe2805ede5f7465c03b7112850d3262a067b9914dac";
  crypto::hash mainnet_genesis_hash;
  epee::string_tools::hex_to_pod(mainnet_genesis_hex, mainnet_genesis_hash );
  set_batch_transactions(true);
  batch_start();

  if (get_block_hash_from_height(0) == mainnet_genesis_hash)
  {
    // Burn 981 exchange key images
    static const char * const exchange_981_key_images[] =
    {
      "443e7400b1e48484df92170056e51ad4a75484db5b470f4cda84406080a8571c",
      "c5024665beb72636c86d9d49361ed4a99e028bc8b7e17a4ea01c5bad1b8857fb",
      "25379975044eaee4a74e2e01ddc8fa6132c4138d127f1c57e3fcceb37a37ab90",
      "91cebf32c03fcd3556929fb2961c386139acf0a69f9427aa6c34e210b8f68f44",
      "2da7965db1fc9b1049293236bf394960897ea50c8669be8e4ef214f893dfc009",
      "fadcda3816ff6892860ee380ea089321783d7b4cef735c6ad4c92bfbbfd4b215",
      "0b1e7c5687daf233d069f99bae6518ab2dc8b9a0b100187ff0f1d8262e7a0def",
      "fc2767840d3f8a6a70ea3bf5f32ef65059f53a89e4b226c4d46cb1ab90a4c0cb",
      "63c2361a2b553a3ebf3378d9c483f87bf384f30ebf990fda8603dd05391a0145",
      "a846eddcd3676217d136a6174267199dbe68681329f7513f5dbb35f57d077e0d",
      "a7deac36ba49697c8c8b07b8cee71a16743dfdb337cd0f86131a996656a883fd",
      "6221922e81606a1a66382ee7ac97972651337733d579efc05fa6c1faaf227b11",
      "64ef6d98843ab4992a7c5a3f0d2847585c44791ae19735d2a12092def1a0b3b3",
      "417cb7a1b50a8ac852dff1424883ba8e95f9fcc1eaeded6701479a8657549839",
      "97ac2cef83807ddeaee7c9a2f08c2efdd21990a3f137ed45d50885d6522f58be",
      "fa18eaef4c51cdfb15d63626b7d575bcb5a77edfa5cfd3cd7588c4c2cda54cb0",
      "1939d16d86a713885dba018864f7e548e31cf39617d33785c60fe67473734f4d",
      "d455be2cf5c88678ece123deca832d0c65ac61b4b6ecc960972a871e7ece8905",
      "afb6f94971be4b847ffb339e705b2b0c8d48814e695a361dc584dfe6c88b3541",
      "402c06920a0ee9245c0d7810a8a8ad03ab65d5c0e1a6945a5b48112b860d7e5d",
      "b7daf4452ff661fce95e5a3fc6c282ea373b0950e9d408c223bd5cedcf67e4a2",
      "21f459c6014153a16065dbe257224d5768de56d6ac4286c51e6e05fbca7f30db",
      "e5141991fae7bd9104cfb2beda0102a3851ad2591b64cce7bd90114ec32d23c7",
      "e61771dd5385468876f21f758bbd8ba17e8073142f3ea9b0f65d481608af8a89",
      "023639aa3ed16ff2e60ba67500ef22f428641cb7186e5394874da4c29349a19f",
      "479e1b151f023dbadbef9f4d6dd26a6d4893139ddcd045aa3b576530f3a7d582",
      "1452a3f435899301f0947649c509adef322a6955b7475cd2081c3c83d3e94a7c",
      "22db247eeefe9ea9671551256856dd88691612585b473be2945ddb312bd76530",
      "b19307f513cef2c57c6587e0e2349142cf6d11b04d2ee545ec1037853884f157",
      "f8b58f1536faf9049267e013e0f05f18a1031e21d5f78a52ffe807d91313c5cb",
      "4e9ab589ba4caace70219a0ff6070a8f2b5b023c154fac3d89a12faac8547de8",
      "56147aea4f273a6bd06767041a00b36df3919ed85d8904566784191649252490",
      "63861f0da90e4a9298db42ee064154fe634bf885c43b4060a896d63be455a965",
      "f5eca2aec3e80477fa35d4af43b0c074ff64a7d2af8b8da924fd57adbddc404f",
      "2268644b870c9601a79cd8f9d28900739dfe1db39c9de5eea21aeeec00433e22",
      "ce19132426da355d8f2c3642f451b126664754e40fe44f67608b85858374bd35",
      "0556badc8d814481c51c5f004aa22b0da4d492754ad2d54dd6fc8762d6794f34",
      "bf93c87341afb12314b24f9db222f71511c99de0bf3ea861761069e980a1fd77",
      "107ad665bb7dae0284747ea834689f3891326cd0c902b601d25cc89d73a04a00",
      "8c5d9af5745178ce91895c9b55542a90c34f4a099817876b6cbbcda45b191884",
      "9b015e726a7ff939081d1c728d689822dfb60018ee1fabd264a26377d34df0a5",
      "9d7ba1dec76e92745bdf3c6f36c18273b19a8e59e7b71b51ea1cf456bbb1d777",
      "bd3e43b2b8a09fc2f673085530a31c3ff657cc7b6686de0d3deda3639742670f",
      "f6accb2dd70c267dd10641c188835ca2d776f4f78218a937cc038041f516388f",
      "fda2cf4d5d95a447a2f05f1af20c356c2abc29eb7a1290f4ba7075a849d89975",
      "3fa624e9e4636061ae94e348a39f7a3c814367462f3951255f6b6a7157ffb1ac",
      "ca0f3e68bff4fa4ceffe1867e6a3d1672f7f6be9963c8106694aeb2444bfd4e2",
      "1abeb3aa064dc17269627324604b9f7b54d184f167b7d8ae1053f09d955e769c",
      "352a6330c26da70ee5b8e3e8a25fb410a1b6f41c7d09062fa51b070adc351761",
      "55b42e5e39e1af330f6acf1c1f32ceca91130d7d71836d1830e0200fa9b1b58b",
      "cd2a35be19bcf2fc636d2dd6a5f63d51f69bbb80873d1051afb0ce20ffa452f7",
      "2f082b1f0d519f89b2cb2ba37cc24103ba11fa636374fca1e1b044f6129bdded",
      "550faaa376303fd6506795d238e0f8b48d0edf0054d69ede2f398314526533a9",
      "f254f3853bc27788d76e2b5b932cd59a88d1d88817ab923451107f412683fe1d",
      "c5558fbda3e59b918fde4868c983535cfddf046431e23acd812d39818ac7216b",
      "bbb4b0d294aa0e1f6e05bfd055e42e7f2d8d6bc78639be3eb687f8b6fdc1e665",
      "7c18c8e63276ef223701efd3cf29579f6d1a3dcec20e3c730b292251e4613d9d",
      "a04cddc3e5c6f206fe2cb28c0ddaff613910b7a68bf71c71af8be45b1763a25e",
      "9ca5b61cc97177480ca34c59926be18a234cac4cc5d28fe19cb2d6812539cea2",
      "f7ba3f229b3c9e73bee85da8e064136e065899c96f1a3473694f73101e547257",
      "cbfd5b40dbc85b7fd896301db7f73a5044fc8695b4cdd1a3efc88588ed33702d",
      "8712be3c2ff186b22150aef738c269becfa397d6a89cf0519888df8bdcc300fc",
      "305252b046e4d449ec87b32f0ce9b8a5fb9126a4dc9caf75e30923bab47bcf04",
      "7bc41fe84a24fcd06a9347f22654fa0f1b79b88ff77e7e55187942d34cd7e3f9",
      "dc4ce4240ea0f6bcfda8a910e9e4eff4f41cb86529eb000d4fd2b3e90ff672b1",
      "c675e0c3594b6fcab154a21b5796f66af44757dbf32515bf66b71e5fe07a3bde",
      "4b76cde6527255220d7e0ccb71bc272b98e6410ecc58ce82636b42da3064f642",
      "64a8331a3cbdb48e941b69cdb5c65914324f860e19871028375be347ae1065f9",
      "aefdb1a2258fcdcf324eaea1b42c21e7beac5beae99c69a648f0d879681b1a27",
      "9748961c0054c81dc5fd56bcd85382cb081c4f41213ea18510c0cf665f5cabdf",
      "b89dbb3fe38477cb425e2d607e0e4d35ba6502bcc6c36d656ec7e2df50d16f06",
      "22378ec48c75aed245482e3d8d1e47bc5dcc2a8bead4dd849638a5ab78d80ce5",
      "1e8a36fd96e3a4c8c5c38bbe5ce926a0bd33fff01ff4b4f8ea765476a5a25d39",
      "6c97bcb2c4e0d7f8c8a0c0b3a60ded462f540466d43ac88b088d5fe49e2ff366",
      "e8ecf97959b76fb0e2a94551473134c8fbfb5ab3c48e99b5fb6cb1a51eb33d03",
      "6476d9328cdccc17d70076178da99ab0569bbb604deb58f3a2c7d855f244e177",
      "c21718a6617df2ddbd6c57b1bb7deb431e1702708d4e7156a6ef0212f1bfb1b6",
      "f6a8c1bef978dc9151ab35155b47a48468324487cc09e15480f154cc61c5231d",
      "a6180096172fee41f0b7e3c72586182986a13b85e1356e7d7936286937246d0e",
      "f8f43c591d113f441573afe367df6d06932df957492676daf281a8cc74a05bf4",
      "9a8e859b52fee803d50427d4e317ea63ae72fa18246f33ee53fc20ebcb84fde2",
      "ab8f625f995e055f08b3d629713b62bc91e611664878a43b0e8e6ac62e5cf84a",
      "a61b79b1ae8cf6ddf580d63de312b39fa0c03dee83c2555668437e09481c4dd8",
      "13dbed4bf969b17d2cf3628162be2439d548d50cd6cdec91db0037a57cdeb9bb",
      "b5c43a0c8dce5899881a995c14503ef60a63ee3aced710591b6c5e5bafb324b5",
      "233a8362c135c25cac27b9751203b2b30cc14b15ddaf5835ce994c0195fbe3b9",
      "6ac0834a29c3079ffe2efff1c205ef6317f9b62c6b62f18127da585c594e0da7",
      "ae786a8be3f7ee6bbc728a0c5edfc24a21323d9387f29e1fac5bfb9d59a8fc94",
      "b692ac6dc1960bbd865d4497e86cb38cfe437d2c368583b0f82008e30e303d6c",
      "ccc899da06a576c00a854301c5296d0d21ef3bb1fb8b725d04b2fb8dcd2cdd2f",
      "24eadcadda0dd7abc1b94b54df7a086323bade3c04757a719587f2632d3df83a",
      "504cbf15f4db3e7b41d5de932f9dda5f97fc885b9fcc6cbd357d0add635beca5",
      "9ed5a97a5e7e1e2f4d78d6cb68cd5800d68c358196f08d71876acff1d9a40d02",
      "51e775e0eb2fde0a73edf9d3560774ba06c540cddce281afa2aaab6e33df697b",
      "39093f37071650cfa03ee2640c94f50891a396fba0aebe2dd406e4efef54bb92",
      "5302bbca8ffd8d83ef969da102b786a04df58c1365ebe2152da107bbb97580f1",
      "65ff4963c6465cdd3b8b48a522a1e43205469f68f9f85861e1ad8932ebc65a3a",
      "68037228b3a798c65d7d7e2f6adada9a129feec35fefa470a0383afbda71fca4",
      "7ef5d1d216b6e4202ee1e2e1f3b5d8e7795d027204f89066830e0dde7cf1fee8",
      "2742e1e21b146f54aa966f0773d2cf523387fa5f7027f63476073673f2dd4e80",
      "47ce372c72ecba5584fad1ea4722f708e96e01f26d70fd4ea0b0c5de26dabfe9",
      "9749551b48705cee5ab8cbadddfcfd902fbb162af58be875d04de2c1b30ebd38",
      "0441a93cc3c0c6f559494077e5e89775adf9f0c815f054700f5b08039da765f8",
      "847adffdb49ebe5a19f2f73bc0f301345bab0a281e3bfc1f322cdd8186a4db43",
      "a111d965c83cfb61601076f3e76143979c1f1d9c11ef3536381d5c3016dc350b",
      "01507b81ad1b6fac29db0943569b899c0c56bc4bec7a2e72e308bbfccf0378d5",
      "c98b1f35de69a5196a3dd624715eba25b0c57026bbe9fe2ffa9040158f8d3217",
      "32bdc04da38c22c8ca18f7257f7f2afa0022e25c84ab98adb5d7fb0228f72123",
      "974ec614acc35b0e00d049a0c4917697257968f5e54e8417c791d83643a879b4",
      "972fd6fe82578c8c5961d23538cae5924b92301308749076f4650b5685851a4d",
      "83100235752fbdf62090cacddf56e99aab633411f81568045d2131a858f38a05",
      "264faa8ff772fd5cfb2308c3daed34d52b00787a8ac0020b53023b1688efab81",
      "649ced7013659c597d956cdf489978c8be15f1e071b7a8790accc83189df2ee7",
      "ffeb73cc383114822a160c5719b5cd28446de1937f468427102b513f8d9eed17",
      "2b6fa5ec08703d31c505640d12f75b946e7ee21a5098406053e687da44e7e1ca",
      "18cf17fb6c23df777768f83ab29a64d8456b2929c2d244ee78e3fea96b2ee740",
      "4997f781d3e998963c4e1f90578f8b06dc1df6d0e57ed0e8628d13df67a3229b",
      "a23be50983d372997c55fe3db70a08d4677bb8435682e0d2306edacf4f596e62",
      "edddba373bbab3e3d7e5985090f683b213eae0112619b43b40542f7086317bd1",
      "2b4d52f34f2e4af355c21e38fa08b79115da3a472a06984e5996bd6b76c60359",
      "acdd84fd198ada3aabc0ae042aff99aaeea5d1fa64e6dcd95af6087af32e11a3",
      "2d890e645261cda7d8a53bd6e9f1198f03958b9bbe720f67f71fcfdde5e89d54",
      "aed033dc118aa363f4322f4df690da5151e754ae7caa2ebe5c8b896280f807b4",
      "ffadb6226d185e782895e524a6e0f38c5e5533753e92e1c164b8925903e68591",
      "3a56b81a4054346ec3fbefac835a6d806eb2ab6c3f60eb3fd3ea46e3ee37b196",
      "9ab0b0adf59b2b095200bce7875037e01c30cbb2d8a43d820afe34060d576c5b",
      "b02a239fd3e55b9e368f1053ad5929a2f9cdb5cb657b034e293e1d094ac8aa64",
      "d9d17f42813063cabe2bc5b67fc19c3d4a69152ce8d96e26a4b19207a08bf903",
      "cb53ddc735f32de6fce7156fa1b2e140a47bdd74506b7c8d0ed2923dfc4d9d34",
      "1e34dcb11841e92c7e96d7163f705c8c3d652a9bbf52bd8e3451c84f1d43e9fb",
      "acbac248ebd9fe6753933fa965cad483959dcc1bae80a4451b55aba5b8b93fa9",
      "385e836d183e240063d60023812eef12f63e4ce73f443dea32b7c9e6962d44e3",
      "330296af7af595221ec90754992372f56931474fac6fdc7e5a0f5b4b1fe85133",
      "ee9f13fcf35d2029da58032cce3498ba032590133fafd4c1ecfe6caf2b881225",
      "b52cb975acec142bc3d371c84b2a15b29e506f21eeae91c866b90bd43f0e954f",
      "0db6cc44f3ba6afcd0722312f57b9493b520c962522de8fbebea0c696a3f428e",
      "ed18f980a52a761b467e7cc4fa3685d2173c9aed51b434a615f1fb7dd8923137",
      "fe17a3d2c48011319c2caf0522c5fe4b752d1192ca288d2b0f2b6557e18011bd",
      "f95dedbc102a2d5b4407cd3e0d06c615d9fd8a02d7219a1f119d58855643cd85",
      "36ade5536b7c179a872e1a5b06cfbf72932b938ea35a79c94f9cf63c7e460f63",
      "1a537ce4026c96bbd57e0c08c3909d046e5e8e3ca4a917c7f62f8e46c096a5d6",
      "7586622cef72706c2d8e5886bd20c08b5c66183452d492683eb13bea0c54db37",
      "a2a651954ed98483b082a8223eda9c85741d0fdaf0b3b738de3e7e0a1ee8207e",
      "d0a3cfac2ce7f5b16b452d57458c29825f1af292728614dfd283cc146c39c6ac",
      "c227b713c18ba814bbdd85a66d50ed159a65a885c0b7f048db3aa612456cc7d7",
      "ecb6a4c272a4c5146e416dba4d789ed5e49ba028bb49c23b71bc1966182886e7",
      "4a40670e49787847f51842bec7c0ac6be191b876cc5ad2250a498dbc52c9a4ac",
      "4092c1cca9f25bd1a1dd4c8758760a451f6c8ef679d1415d45b7236d9ebc268c",
      "456fd618316f4b5feda89251ddd1c55abd072da91da264c8ef249c9e5cdf876",
      "478a3768b56e90bd0a385c9826b5582a85b44156cf87ec8992f5a7e0993eba9d",
      "05028caafdd30f5787f8498a7e5b52e47744739b09669282cb01d71eb2396374",
      "1e392928beafddb0eb482c910f110c9478415585f9381dac3edaa9ca5caa1cb5",
      "451d762d2ae3fb719e59845ee6079a5e0fd9d6cc8dd74516827f78f8dc68c31d",
      "4823d9635d70ee2962f642bf22976b5dedb2b1401e5f091b107f80fb6d8dbb67",
      "b40bdb970f0d2a414d8b22b034306e68d7e0795fb073cf9ef46cbec25e2cfd1f",
      "79f74d6696e200067b9f1917eae393105a0dd0b4fdb0371c786f9e982203ab9d",
      "7893dd31af79706cc1fac74680ecac709fac49b59be986800d881677a4c97fd6",
      "d82a1f47c7ea42e56cf7b1c0943ec4574ea773d9532af9afae327098fa424ec5",
      "2ffb1373698730c6027cdc772bb27d0c0aff6a4f8efaa64e57f9d50426bdca17",
      "97edf5b058d51ef29eedeb5bba050bc307d29de26182cb0dd070bef79af4f1ec",
      "6bf4264226f780520415d336c12d54b014a18fd1219feb7baed74fcca84b8c7d",
      "5c1347a0c78a2238e9c8cf42f5a9d52c08b28ad09c023d63420f79ad80b7ff60",
      "f97d80d92f669885c570216a1163828bdfde2e1df80ed58af579d8dfe3f0b48b",
      "ac3e9734a5ff8f19c9bb1380f9c45afb1cb76d5ed0534f188df4965b2914854b",
      "78588b9f2a5897423e0a69cd26ba97ef636945747e94d681db623a2649234b63",
      "9282b8b0cc865a299dbb0162b1dff106843592f1823db0c5139dd59f2f65bbe3",
      "6a748a6716f5726823ca091495f00ef34b6e4d6169b792a71034284b9f2fd613",
      "5ec445c01488816602a39aa3de29e15ce4e8e40251d31fce5d53d1b612de33b3",
      "65c3e24ddddb0f9caffd668ce2167b74ab2fa1724a3aaaffe51ab6765fb9cbc8",
      "674081adf645123fa8e1a1e75693b25286a6943d048304c54e64398edbcd98c9",
      "9328ac270ecc086e79e807368b545ddfffb3abb0ecd052d7679d86e698016795",
      "62329d408f452532161723c4feb8e56b6d004212b22c1c907f044f26d55bf25f",
      "7c650b8e75817165aae8b431813da5345f91c6cc13bf3f63667efa4b3b2cf145",
      "2f52a191abf7d63610a27c9ca323d5613e14d91e12d0853b2f0d85797b3f09e3",
      "f2921d61528271c488e2f57539acda420923eaf3d3900a9045a38691ff7e909",
      "66751fa1b8aa06873fab9387a0595f450c8b16a74752d2f0950047c0126d4cd3",
      "3269c41f30178f949bc9c77ec47108af5480cd22b49d6b6d2bdc224d4cc8dfeb",
      "c1460c89ea4e318f2233ac6e3f6cb1e3c1fc4ef4ec83ebd89cb38ef6ef547326",
      "30c72e6c7a4ad8ba3fede29e39783de01c8c752f7a5865a56cda49d64472de52",
      "63bc2a36b909a5178b38e5dc76786c0950dba111052f408b85a2699a38d1df13",
      "7ac50486f571006782b6997fee8bc96157827c0472b3e59690bea5315a87d40b",
      "df4009f6756418b14a864c13b62e18c7c7949c638f91dc1c2a0ed4c93709c272",
      "59c8092c0aeabacb875f3fa6a32ce79accf7251e3bdadd463bd63287f36c85f0",
      "433b8e5f325f1b80824ed05e528757777f769e6ad7f274174562fcb6c80377d9",
      "747acc4be5812c588d2ee3927c95ed6348af6a6326c7b973f6a53a3f2478b8cd",
      "056c0621b4d0460786841c60f053f339b324292d4d05fe552da689fc083af04f",
      "c57a12d2f00000f820c12c94bc86ea312bd8c82c02fae151b1e6eef43b09879a",
      "9c8e619c7d23584d11026bf59256ee39122170430933cbc6fa53750b52663f21",
      "9d46a8547415e585b2b676d0f3a6b7600a22a3c1aa22e1f074b39acbd407e3e3",
      "01b6664cb2f5e19d251ef796c9e99d5ab8a2f9870f6445bbcee8a2943c6121ac",
      "a12e5714a4c1706346cc79ef538b93db12496de4f9b56bb260d5fa9cbd582f30",
      "db0f1f2ffa2fe027e30a73652249164c4beeff6815b60beafe2b3e8b4a2a7c60",
      "ed1838d8ffb9d6dea6ccb06327dc1495e3a0a2773adebae432756082a99e72b4",
      "89ac964819595949c74c3545fa614128b36c091b6dd98d13d610cafd6eb1e493",
      "baaa854f8d695f54c0eba0eb5025e45d592c294adfa35e77fe9b190e8a06d709",
      "6ac51658e0cc0e7f7beb14c78cda25a78b597d21f90f38701de1c452f267acf8",
      "f5533d4b9172f9bed9061c661128f13734654b002a9fd2ad5a8e321781a7b6fc",
      "02288afc9fcf7d48cea00de81f482d5c2f904b9603602f34431d6a72d187385f",
      "5f37b3b6d410583e7bfa4606a2278e258001102ef397d9ad25ef1894859606fc",
      "856b3ace6d9beb3450e12a512e2225c005199cdc9ac5e7949cf75b574cc1b7b9",
      "26a5d46cf2f5260a54578afdfe2e2979fdcec30006162036aefa1e7ce9be53ad",
      "5346b8f15fac917c7b668453576857ae460947d7d7d0ff8c754263a208393fd6",
      "0d8fe3c33ea67d3f23fd638eae325b32840621bd6774895cc5aa6a03c3429bf6",
      "aa02d5737ea0b52f3822ede2ceff3b389008dc38b14e71dc457e7f0c7410591f",
      "3d9a8949e09b27329974d81363365bc4c0b924b1c55542041879e067a9b9dcca",
      "d85e031d1a5ec35d7794d89de622f545a68a5c65c903f4b25fd6a1f67cf4bd9c",
      "2ad58fae21f4c13599bfd9b47b21d59f319532f5531e4ff41b3502fd9a09dd15",
      "01a9eb864c9e25e841b33ffad4e36f675598f39936c3eef1af8ea28c0c8170bd",
      "999290f1797b35072d340f5df685df00afc6b534ffc4b4d3606e35258f010efe",
      "098023c5b0df9b484e2928aead8a4ded7adb9b0d99a61f955e9f053de3c2f82e",
      "9e954ee6e47d48e58badeea288a222548b838214dea69b39ba1299294dfb591a",
      "fe5a8a81f44682bcdc60af1ead506eb5dbb6e0aae51627b8c783f786a545f703",
      "8e0c998d6521318866b978af3094b4c2a355464c0f2c21652a1f77e02a80246d",
      "5ca4a47ea08437094014cf500e7f085fb49b7993ca96031c40d4f99a24fdebea",
      "842ae801c63e26b2e2f6cf22ff67ae894a9a528e7073e0ce94904b7ab2d5d306",
      "193717c8d21f0a9afbfa9efc2c56c8b34a6fe3c9093160b6f3aca113937ac0f2",
      "852fed23c3336ff3b63b1ce7b6e3322f7fb307763224e1c05f793cc3df8d6d92",
      "a20cadc17cc50b237515ef08cac02e15a236efbf66c25bbc4e4f61e6ac2aafd1",
      "b584367264d64856ce90cf560215983ed91abbf7a85dbc47d32976419e16b18b",
      "967e8ecef838a6b357d2aa854110c7c8b9b55958c1a56b7473063189ca926437",
      "b988f96bc2e321abf14954718407852724ac17e4f6a1c31c7382d9ac1a5945d9",
      "8c8c14bf07daeb7f4d13a2068026554fb80c6041b103bf387ec15eb92209feee",
      "f1a81b1b97ca5d96ba15513f1917e6466eff7db1e85a131fa39b0155683b5883",
      "76f49ef1cf4aa4aeebc2b1ca67f260eb1c51cd25c198f295a9e1bb046d328fda",
      "7e2e434ea70941063283c1840b425336f91f0ebe0c7ae23ba4239e40578b94bd",
      "1469a37eec9585f17aa0b1da9f069b186519936dbb465a497034906008f5f7a6",
      "a85e5196440e5d7308eecd356eab1291a192b923d5418698c5b9322a6c8aa5c8",
      "eeafee7693584a64d9b6922e2f6b8ece0b342ca6d3accbb01ab6955fb32f2de4",
      "0ea6f28fdd859d3ecbf32e8ff76f1c7587a3f05bffc3017419f0b96918760377",
      "6a8ee8bf353a0d747dff47407e313e2ad0f44bddab62351f0baf56d742821c3e",
      "fa7d02c58560324c93a5f0d3aa197690470f4300a9a167fb88e866a41aeaf2e1",
      "46f60ec4b08546e395a426b6a2f03a7861f12aee0eec37ee698a4eb8a0be447d",
      "9b099798afeb1cdc3dc8d91858402b0c761395870d9329ce75ed9ad4403dddb0",
      "0144901ea8a84a67e31cd0089cf35175f47b348e86f34407b6cdbf922601dbbd",
      "e3864e02e54a034fe3c8d82c8283ac801f1bf55fc3e1237254d8596232ef6730",
      "c86637d6b0e7a2b3364bb32df46d199b485250ad5008f28298836be711ef1a77",
      "55a807251fb353a943eff1338263e10e02d1145e7ff570066ccc9e0281f43eb8",
      "81258c4999d8f49de1a5e90e4aa5d50556216c83906f0c67efec9b8d7aa27b44",
      "e8b9ec9856451ced4908ce1c00a040a0a06d867b8cf7437a5164052d5b698457",
      "546567c5e305ee371bb2f2ca82845e4d31fd0a8f9ee757273bb1f2926b0ff8c4",
      "ede59ede6d58f2f7d1f11ff8d3fcb1b98750e283c3074994ff33c5dead57e7fa",
      "25d865bba501a633490db042780d9297b86842acf6843e2b13e09d76617d286e",
      "23149144e18282eb2b68b8f9244659f0983ba3cf5e165b5eb6a3279b0d327c95",
      "c3939d389d648a90cdeceac6b12dcbb12acdd2116da33978a57d20cac906dbc5",
      "a6b08c31ff4a8f877868ea7b61a32ec8feba690ed7c4c23bd008712da914c718",
      "a4a7566b31323165b0ec18a5a4f062b4288395081762cbb182a86c23a968525a",
      "0eaa1116eb31bb00311ebc7b4bf5fdbd0cca73f04f62246bdadb89793482aa7a",
      "951acebb9f9368e21e96a76014cea6986d0a636cbb58a496c268b8a469267fd8",
      "d41dca0310579036502e09224d9b99c4f9e792092cc4c146bface14cb571951b",
      "702b385f997199c88bf0352c7d7645291f1329ef34f804c1e725cfb504eb2ee9",
      "1c56221ac706ecc02d3f05aadce5f1cac5a13a91110c4c200eefa11de37fb7df",
      "3d577db6c6dfadc0dc6f1da2140ebf21495a17667a2a4761539da6b09cac797a",
      "dff29c6b18428dca419faf8d625bd0d128386b29fa3ca119e4565d5d933b2a49",
      "bd663af359a515568b242f973c7a985296af215d5ddeda36de78f026a2e98e9b",
      "463337d1002040f7b426a43b0dd81eade7c41f27d29b41fcc3d9e66196933962",
      "142b5451d0b3f30b675c80f818bc90679921f2eba6501e14736251609d3a6e62",
      "07889b661c38211f726f843fffa304c51445574a54404d0843f04ba5f5dc791f",
      "129f57cac1a27f9e1bd3c5948523124e8eead63b383e58d9024c3a8149b3c9dc",
      "d42cd348e0afced3e4a20c948162173f6a906d3514a74a5f1e0a0485e7547ef7",
      "6964e450f930d28bdb6e200f523edeabbfe53ea2cbf12de3a68012415b61568f",
      "6aede900c842bf692b75438ac87281e26fb51b4033d0d17d982892855023d156",
      "c3e18d1e9b8ae9772d063c473ac62590794bb3544b4111f4831161185c0b5c97",
      "4f8e626bd1a4bb0986a437eb65276fd6032dfa889d05fdc27b32bf2c4785b94d",
      "4fa3bf6b6ad80477101054a88b73cf359a7238a9040858f2a35734b62e516d7c",
      "690997bc023efed5fb4a5a00caae65120c904287d59cfe3f899443b48861f9b6",
      "ccd188b007704b4db5ba2096bb697b56cda6d222eec4b3d583ad231ec93e34dc",
      "27df5314b19bb00c46f6b2cb0379a176a67828a5a6acced975ae8c03dc197875",
      "e0e2f6ac9d0bb4373d52c24fcb1f4ff747fa2156326311edc73245022b82ea0c",
      "2366542acc0c720d9eb6a275039d574aa1a69aff6b1baaf3e952ba83406736cf",
      "a07d2031a2286d23889010b26e79dde687b60bb62e4aa8d8c3f69a11493c17c5",
      "1c7e0d4fd9d33e19fa62a14338abbfa6dd8556901bcf6a54feca297082898bb2",
      "9eb80825b368fa84894e8f0f8fb98d79c4b83edbf90aadeb3fcfd95082ce4334",
      "fc708ad3ee99e4615e48c4f529b40682704fd767ca182d4a5f31b577f0e731bf",
      "33364f0be39aadd1f4dfad40aefe2af683e8489536b7502e69b0bdd1ffc2b6b6",
      "ab62f9b28fe535c2458e2bc5ccf516f88d9354e7547179704372f7a7996aa448",
      "178c986e3c07871bee7e0ebddfe5ddcea26d47a72cc8bfed927864340fe7c033",
      "0bafe5253cbfde14ffd072f57b44ed44006dd2ef8c293493d62d04003fcc1dd2",
      "3a664a46d38360defb5194dc9d53adc91a629750515fb284982e9c3e1af57a2f",
      "8540fdcc1edb78c30993f91ab4357793cc47dac490fae2417d347583f8abd4a1",
      "d54bbcfdcd94cc23a03410c584ff51d8c9080388f1f012c8e40d2c1283f79cde",
      "47fa89239a98c0402e79c977e747513f6f50c251cf08fa7dfafd527c066032ad",
      "5bc68c403a5127f8247cd6917f690bbab61e960e8ea1696c71cc1262ba6a5aa8",
      "6d6b97dccd60d29ecab1e9039884dba58d094deb20d0985b22c9065262a8d50c",
      "ddd60a9d0c83a7d0c22667e8d3ea595d37d70ab998b6c86754cbde44ded1b726",
      "12c6c6c5e4b8e67228244fae56000e86d356aaf0812c6a22d18e7c80b0ce3228",
      "ac8bb05f52807ff7175cc37905e7521a5ada58bb337c6318502fda7b89783d1d",
      "b01ad5994464f7f39e908bbd01dd787251030910829411f4139aebfbacae5741",
      "27f7f180308d04b0eff8d64d3ec156ee0e02cf9a2dd84560e73f72c2915f8d89",
      "902c632f8b032fd768c1fb3d8908ede7cb511235a0694aec941f670b647134b1",
      "3eba17c5fab7913bffadbd1e4806b118fa77206bc2efb904b83d249dc44e3caa",
      "f3348a3e66f8d5bb126e4da153337ad464eac3e9bc2c09afc83dd22061350ca3",
      "6c1a4473a69e7eba72a563a7c1fd63dcc602c0bf270e156f75e10340649f3421",
      "77b156581181f4f6dde827a081930bb198fe4fbd88524d2e07f5352e81b0aebc",
      "1b171b766dc04590d22a741226ea1ad0563d62cb51b49e0f6285f0c99fbf8a5f",
      "07ab48bc8ce4d9658722d36b9d724430b3620dd6afa69226e4230feb83cd858",
      "57d504b2489c47e0d50047a015c11e4549eef2a5e485d5febfe705a0c03efd03",
      "700e47f32ee7320b7b6b601931e6780635ca5a5a53c2264992b5605c87027a9d",
      "82ba341a40e9884715207bf83385fe5a06e61660cb227ad6d89ca3f6a2cd52bd",
      "968ca53c30a63fa05e3963dbe7d75a56ffd6910d7b9d309e2563ed686d9e9a32",
      "74a7a671fc9166152f8e0e223154f876794d0ca4ba17ec0078fdc37c6133dca1",
      "3fba90d36e4aecf8fe9e57fd4d9e8fa30ee8a8b727cd5882f0e89b3d4ceca370",
      "c9fe5dc2b93008312a497f35bab10f0d2be4a10ce35f808fab7e76501754df83",
      "d832ec0d792f53a323709099dc0b1c7af61e42466905f4a43d8d42d2aa62624d",
      "3aec368f18167e0fe2be8e35bcdbc632a80b8bee272fe1ab653482d58b00fdf1",
      "762c7a6084731060eaab9b60d90f242e662dcfd0ac5f1581087ce22a66c34032",
      "58afc7875ce5f99eb799fec302aab45eb28cbe353ed7a30f72e853af3892905b",
      "265b76162ea42806d131a2cb7794137144f8cb59f93503e4d0de03d086606db0",
      "00ff8d71cb3aa8af44540483d843441c26440575aaf3e6f89c84141075269a5b",
      "5dba58bbe784ccb64ba900db2fe3d8e841f7fed98dd6044a9bfaffe560b528f6",
      "244e7cd00710267ef02ab9979c723842cad54ad6888c22a73f3308bac3985653",
      "2aa8823f04d44a1e294a92323e7386da35f347d6f92cbcb16607fb65cd442018",
      "80ff2dde4d6d99bfee12023efde6b98890a4b94b6687fd018f92e7e125f0f3af",
      "24b83e357062343c6103da6219a0c4b6f2076353816ba8334f9c4ad70b1e8dce",
      "063e94ac569448113cc8a3523e22f48ce4d6f8f271a20976d7ab0a81d8970025",
      "72f5e73a06a55aee4281b463ecf43bed7b08da1700dc01adf1f1678339d7c1ee",
      "ca7424f4793588dcb11d17f2098a0d50942a083e6ec6e3c6c05e2259e0e55bfe",
      "73be5c0251a4bbbe50a0d0e05a1b79d0dbeb1d1d6067d4898c57aab7a48bcd91",
      "2b92e63c4ac4ee5db50d7d59efeef5ab9694ef5675f47d41ab0ca47f990de336",
      "916a56c0436945646de94b6afc88b90f93837b6431c0dca54c6272f0db65ffb3",
      "5cbefc8882adf97e2115539cf9a1ae69b806782d7b3d20045fe69917a7fbbf82",
      "e84f1bab02d95d2240f0b20da7f38c2add39d29d31890452cf2c5f7ebb2ab159",
      "42d5f10ad85ee62c6da13aba84e00c19887c30712735801b3d57f539efee64de",
      "b803b1e6e535cb710d26f3b57748f0f32cd6a0945c9c8784eb919a4e3173568c",
      "d0ac26042effeff9beddd33a24dd41064c76ab69271d2545dfe293d7f3e3c06b",
      "310f1fc78ff23e4165bd6f5c8127ebc90e23ad86e6928b37ddb25299d7a6c5f4",
      "df05a499942c06c374696025fd820ef978a999b5bd0e4de3649befa04d4b56ee",
      "94f6e95facb85254a1dbcd7dfd3530da08122b3699320ff4ec139d036e652599",
      "64f00d393324ebd691b58efd779e209e9c2849bf75e55a36b89db2b9ec53980f",
      "121d7a01cc52eb3c31458586c56ccfaeca3d2c1b6093026b5a5d675fe20de1d5",
      "8f6ec456e8e6e202bdffde970ca21403fa983632a4cf88a41070ea5d13fd9599",
      "9a28a045c652fa649840f9226e348bdf8b1f932dc39f07e5d8ebb5bc7e207acf",
      "4dbb6133dadd9c9d801e99d7a41ef81d0870b3bef911e61313c13bbe78227efc",
      "ce85f98bd0c19333310046c5671bcb832bf8601792711af5795267d66813b9ab",
      "45c2fa34b30f8493825ab7aab102205262557d8f848a84ff66d22060383b5ba6",
      "1af795f4190969bc468a474d9085c8d8c5c2d706bbc0c2032f16422dacbdd269",
      "8e7acce695743e0c50248d616694a2980c45333cd58b466e7149e902993dcb46",
      "4942298e3f9b4320ed60e0cdcd588350e6ce50ba2c8cff2f13a3446a0700faeb",
      "eae21f0e212468650f0a6bffca9986b2bf869a3e9de6674a935a6c2721dce410",
      "651f4a3af1a0bedf35806967a595ade7082aea98aa1a711f7e9914c12799319f",
      "9d941b4b8fac82e81d9dfd8f6c99ccf7caa55d11be7e38abbc8cfbfe7311b648",
      "8d6f327932fd3be7c58af80f79a8d97c7ae53dc26993dd9ffee4a49cb2cd0301",
      "312a73b0dd4c98e804750382363c5567fa1a2ce9f8ea58af65bb61ab38ee3550",
      "e05ea7a30262c9b24374bf649ac9f3442e71deecca4559417b98bb9e2867770b",
      "66dcc7245d997dea446e2e7e2bcc11a324b0806cdcd95b49f74dcf2b961ca1cd",
      "a7d6d2c0c4f1fd34be381f03f1f5bd1d6b4eecb82f1b4af92e3df5f2105ef1a2",
      "9cc730a55ac64d5d44d00d92a3f77c13622a6c1f0833577fcf63c0fb056c666b",
      "754a705f95a514efd1f88cdd48a448de53e162940468e1561650160850be11cd",
      "8915ae7a44ac6ff25e609a230fdab5327613d70b0782cab96a862dc1af73d545",
      "ee8dc0fb5c6352fb26319066d4291f0cec57d321d8caa8824128e00032d6ecef",
      "c8cdab321c9e0f0ac6e4031da2bb9ac37ee42e9afd7af6e77b86de04bb2fa071",
      "19eca7e720a542503d45b8461d56f3f7f2572ac5b0f25f0be289ebc85f10fc48",
      "f529c3615b1a1873c0ccb376ab385ac7073749c25ab58971f4bc696492c658d3",
      "2384f0f38abdb6804722113cf4774ef08a96837b5728449d7a5206a9b0975ad3",
      "57341c4f59bec7c907fb9c56057c7b44d3f4cb000a669b1c7ee1199fa2288161",
      "be2ddc14279afb4ed0a4efae5f2a10e3d335685e0488cdbbc765466e7bdbd855",
      "2844011c233408a4c762815037cf5da75bca4c61da479b8eb6ac673b5a249f70",
      "93fbc273a5161418d8271984fbcf92664c97ab0a8699a93b9b7538cb71cc22aa",
      "8e7531629ee6c2cc8a1d58c763b3a2e44bbd1265421202056f1b25c85cd8affe",
      "da2ee6d34838ed3847db123f0bc1bf908e5446735a8c228edf4781a186a98434",
      "cb64990815eac1419c8a5a1c0b7b81c4e860a6a74f33c748ae8f22809dd00552",
      "6eff0a10596dac0c077c0eeeb7a24aa30b8934364c579b923adad6ce9ff64790",
      "d603ffd812286d33e5e7623be632cba2c513692cc57141639ff5b8680df89c06",
      "1bb7caf077ede213f92364c9d1db2c99d3ab7287812a4838f49088ff042cef85",
      "7d3b69a24aecf6e13fe06ae8c38f9d3fa72e5183ffdb7c3d500adfecbb1f42eb",
      "62a59b8ede7003b10a3411f466866a9c99e34b3fadac6fba021c747e37a657c8",
      "a8f8cea79230d897ae1a86257d115dd5e0cba205b5ad0eff8f91c0230030209f",
      "d8f92609164595391bf297dd8e9da94cf6aebc7cf8280d0534fcddd0d650bbb0",
      "30ae7dc51fecfa3d5da4aff8ec882c3f384d529e1d62ff3dff30bf388610b779",
      "a5a5cb6361246cc38f4497418d5cab321e4f391e007b87fe4e8f1fe9e9285804",
      "8b75d8a72a791dafbfa72d645f6a475014485bc90d62882f303dfd07976858c8",
      "8ba6cf10222201f476d1ac438277a3cdbd0831176ca2b0e82b85a5017a8e9beb",
      "d563e5c9008fbacd6f83b3c9b68443ab6a437b7980659fb105cc1973542a9fe3",
      "4c96e485f796c081b802d2d5620cd18a4f0a162316c2e9851ad9c795db65003f",
      "548a25a5d5844f27e181eaa750453d7fc0461850555e810b91e3f31fa83f0a61",
      "8e2a2c32d62f18ce158415e240888b291d107f8b2047aa609e99f89b2af41239",
      "ec41d01520b4b1128621524a4f443d1b8ae6af1a9fabdb44bae23cf6c7552088",
      "5627e8e91d6aec276c3eff92172d1e79759f47c601081e5f507f80c21b71a5a7",
      "c2dd108e1f9e160edc12c6df2cfd08108addbbb39f9d8d462ba97ea66a5900f5",
      "31d430a80351ddcfbc2d37b0894aff6e574f20173ffa6354f6067d41b31eed7f",
      "5e971660489b975b961aaa771cee25d6c7b10c97e1cff8773d1aeec64516625d",
      "51be9e1b513e272a7c63d52b561b24d34dd29918156982f3a4f6a38e2d195d5d",
      "234f71fbdaee067fe0604bc88427be95c7f7f300ed0fa0a61f8c9cf496541d15",
      "24a0c6be3877bdcfc31052953df78d8bfa010400eb7568fc777ea984f3670448",
      "e338804bb343e6fc35ebaef61ff9da37c52d12b48b7c4c83d532e54fc75927dc",
      "2021a250325a973020b8f9ff6ecfa18c178061364032f2e11705dacb995b353c",
      "7db124c3bd25355fb0efc33e06dcd43f6bcb9fa69ee9f812b2a027e2f2042c8e",
      "ced15c67f32913d48b7cde6103c2ffea8f6f85643c410e0b54527748baf4c945",
      "d9866eb3ea91af04e364d79de69fa351a3d344d88f7810602a10f9987b7ac6cf",
      "daf86adce3436dbf6add07eb6372c033107c2091f73abcf49e45cf9b21023659",
      "bd92200e58d35222eae16bd3a25c8582a9d3b2ae010fecb6df528936dd4471b8",
      "57ec86928b86b5e09a5999d128dce2e25c34c52c71c6e9f41d1d4d821ee03709",
      "043bb32cb64f965a9f38d8dc2cd6928c4f4481fb16b78c8b45fd0c10e01300f0",
      "fcf027d433a5af8cde4075c03e7144ac507218700e21fb3268291f806dfbdaca",
      "f86e6374c6924282108eb23b864dedd81c3f884a3071ca6e59b6f1080d365546",
      "c2e7aa653aaa1e452fcfa188547ced744d083079aa293ea7b4f1a6de80c356da",
      "f6b353df89c28bcbd451e47b0bad1c2db38d0a631b5763936924796595a65832",
      "1e297418480c2aff49813a91ddb51845830a1a97110ff1f08ae01939b0cbea5f",
      "b39fa02928ac869a0b064360dba312a377b7c14dcf3b44fe8fce56ef031135cf",
      "d675f4d0dd13dac73fd2c4c238fb3f69af502909bb01983cf96ca952fb3ffeb4",
      "832fe359ebbf8d0e8740cda9326af6eea79eb8df4a821f72578c4f4514f45b7a",
      "68aa120bf2837d5478265979cd9d4929e7053d32a453cbffcefe7b00c6225591",
      "6eca8cfb9e723482482d550aa17cdde30ac7f41d6ba58a5eb499cc0183de446f",
      "336d64b62564a9f062bbfa9bc210f3bef0f3f1b0d68190e43f87e2fab6c4944d",
      "4c850841f59614cfa90bb4e8dacde3016299e5caa630172852f2955dccb8efd6",
      "9399aa7ebb17b1af5fbbb396c255ad45cf5a79bb55035ee784f0f500f5d44b01",
      "ca8e10f7e332beb71d6cf1837d20d02cb75b8fcf3ac02ffab1217efd27ffe1e7",
      "d3850a4fbdc9096c14fb089c6b9c95a73caca7794e0cfa2c9a09dc95aa9e5b1b",
      "749e04345646e6e6cbe1e10e2ea1b890c6c8abb006140bfe01d7c417850362bb",
      "e102c04858e1f88593f918e0cf6b6ba7c0721390a31f5c48b37db32f6aa34102",
      "290c279f3c5868ee5c263b9eeaa46e3f87b8593386b31735225f334c793e29f1",
      "3e0b796847baf1a5e1c7bdc4296df7bea4967027a19b2b6a1e9d03794147ea78",
      "0fe839460b77d99b4a970660776cb4238fbe8b383f993c638fbf7569e4d19f74",
      "bfa59ffbb50d292533ad543e9c18a4efc3530f740955691cf068b9e7e8b3aa7a",
      "44fa75bacfe78ce97b8f33693ef8d3f1d80b38b394c219c57f5ad20b5c3e2b5f",
      "7947b554c73904ce7448d5a26350edffbda88eddd8856c980b85c15a00600ab8",
      "253c7652abb687b46528f3d3a35c1d507026105b8f6cc340c538867d12a114ef",
      "b53b29ddb12d6b082c414a9eab2f9c8531df5a59ab1e2b045747c2737c0d7815",
      "5e4986230cf996fa34b1c3de264ba2858b36c936f4435de04b98330977d6f912",
      "386062dd6b3c9f3432664ae38c2db8866ca9998d63de4d48a4b4fadb2676bcca",
      "9857b76554fde7b710a27bbbca849b040a2628dcf767faf2b24175c5bd5a1e5f",
      "a7e48a9c3447d852110fe153e1040c8edf1e9df88a752c94199bdf6c440ad7ee",
      "0d290b3da76f7534ce97679ed19d36400d79dc8ca45154cf8c973246ffe74323",
      "e0a74b357d16a7b8874d1e4b76c1a92e4a4335b8b79539ee2d068a9d18f2b23b",
      "93a8d78104ea40b11afca955b65b97564e29cbc5c45f36136107259585c9777f",
      "024bb4131d7449aa1f3007e5a8acb0458feed79b7f2e1f691dd650267d6a17c5",
      "3400373cb2b44f8febbeef6f4622278a30b06c96abdc348046d320c3935bce39",
      "20eeb0d2f45259b06a20d6c0bc14f5806ac308b0fe84696dc80d2d5361b9925a",
      "8a15bbe4bf73535399181eacd6e481775a4779f811991ca482850477c3ce868d",
      "07f2360649fecc1d31ee89b935631426ca3f3935d20d136aaa65dd389c4e302e",
      "bba8f23036d4bc75a95c0577bab4a6e3cb842e5a173424a18593ee1e8f1c9902",
      "b339084f970004e0abb44757fb259f4832c0c20b36d2d251e9eae0365384e9ed",
      "0d0ed85ca84d7100f8de6349a4f2dc0135c0dd5ac254a7f178f407e86c9d78fd",
      "5d30859988c3a9373bd850224aab1fa7c5eed1e71e440cbb0c35e35c076056ee",
      "b80454cf8657a58b0f869ba1c07aafaa5a95543d8b1874b5fefab6891aaf0156",
      "7f66708aac2aaa31c2481a2ad97acc17c78d3465535766440b4ee832c795add8",
      "b8479f93dbfdb71c247e23bd21b3e88ffc5609ff9a372dd8d9ca7330f4d5aad9",
      "58fda55dd93eabc9402e50fac18df566be1b3a15268fb54b95ef61f1c3cbd12c",
      "efb0fee55828ea249be3a2a51fb7877261d672eb65e90c573da83c8df045f8b4",
      "085d315d444e082a645bd7d54b346abb8f279538e62682be0de0e63df03ffa97",
      "bd010378e4741f8e2bb4608306f45bfdbdbff42cbed47adb30bde3dfda7738f3",
      "bd00ec401dc9d9d98f25eb7cebd930a4de3a454837e58eaee715e54fd9a1fac0",
      "e86ac1c97203c49fd4654c52961c1fdf23d0e8b7f97a7e329c20ac17e80c2e5c",
      "de21987a3f6158c860837f8625968102dec0359bcc298d3b58744b12eb65d0f6",
      "f0166acfd92ba30269f04c90289b89e075a0d46d38e9f37abd7df52d16f0851a",
      "66450b5859bcb524ee147ddba833acf60e44d15e9e57222f208f1fe54fef0a7a",
      "f5a256a3e34910cd55c81544da819a88cdd3eb7df0836a6b17f4747d2ea26c55",
      "3278ab40c863b5e54da7b04f3693e19624d0354627330b8c9f515633dfd09416",
      "c68c539027763f56a9983a1e29e002ff22c48479ec06c0e5e69b39bad6d4f031",
      "9d720060c7e9eeae2c6115f7dabbeb95443b25e9df96e476e05e8a1630264a36",
      "c13b621d2cfe1ebe141505c7178c6433a5e40f10b230b3bc685b74647c5033cf",
      "5063e74efb346f440e763175ed168e81e9709bb7be20a84b011aebb9e1b53bf1",
      "87ba492224dd89714f6d2417f35e66dd6e77ba1b81ee3d21f6b89b098365c578",
      "f1a4212025aa892fdb2827c45c3a2da9ae29057e0424bc82ef2f0088a80249fb",
      "902d3fc3fec489589363c742068092c89a7478fdac3c1702abd2401bff00ad48",
      "61f7e8d3f30b96ac6adc41fababea13c1d3ab33734061c37f20b92c92a3a9b1f",
      "973e4c00964cb7ab95e84103518058e5630de11c3cdc1c67e50a6b2085e9cc35",
      "eb1ab43fd1f789c55383b1d8d50a1f45b22cb9e23cd655af1f6b98a3ba92ce01",
      "ea7db8895f83596d589fbe648cac0dc5a3837551560caec793b8e78bce09dd9d",
      "d0ada1d935039dc8d25b4201be689bdc8de3b63341a2dcb8903873fda369b915",
      "e607132b1eb7a73baabe260a0bd6ce920798168afe173cb0d7047f93aa6ce34f",
      "b27e5a985bec75ca1975688530380b7c11d7eb66a8a8058086ec332c615e33b1",
      "d0c8bfa6f4893be26feff62debc0efebcb97a89168519d8922c89efbde05d3c2",
      "9b5925fb4f0ac52de8c22e197ad09f995da965c5bfa06336d9174353d6aab3d0",
      "1e0857cb25e92510253344741dfbe44b4612867cd5c5c72ebadf754f60cb5bfc",
      "f0453d136943371036ee4e2f3c66dc82212fc42104a1090f1c969e7055d27428",
      "9277316aebdcb309a447c705e649b82502853260009da50c38f03389c0ceb0a6",
      "0979ff1cf348f0c2aefcd11965e988b9235cebedbe70b67ca3107bae5e76da3b",
      "470933c84289380e25c51ad624832a599f64b260a8a06198ba833faef2ce7fbc",
      "07d824782718c1e9bc536f6e47c5303d53401105b864316512016c7dac1d780e",
      "e5d7b8f4c5c2f21b674fc80da0a48631fb9df0047e2a05ac588c7fa47abc4d81",
      "cd68185bdf7cf90ff9b753b104e28294a443df69e899ed379f4d86bbd82f759f",
      "f65ccec0df30a02843b14fc3e3c000aded495594059888b88ff51d7f0e814cec",
      "91991d37f665b261c9f0d3d9ed756d298b2b63636d6e376f50d935e2db987e01",
      "a9b584088c577eed1195c6eb80d9e72e3df3f5207efaa2a8bad90ac100f7e0b3",
      "60187cc6667f9c250ffe72b79a2d3075eff18605a8b38267aed259e1adb2b07a",
      "89e7ecf37b8302006dce6da29250c9fd6037b40e904b3a0730cea8d16eb7bb9d",
      "173e9cda2fa77fe676ef4fb075d544ed3bf1dd0a9242d290af52f2025070e8b8",
      "38dc5a1496fed676d127b79c41e8dc7958f1f2bf83c4006ddd4a94f8142a407f",
      "9ec9ec391c6667305ac53935cacf626ba89b63b4b7d1bb1d40d30e321b0cbc95",
      "e125181033720fc5140ca1db78971fe39cf4a4d915ba5239b3c3ce3f6e9c6985",
      "e6eb1524c5da3bf16cbbdca3fcca7f13dbb913b6550c14d8abf6f01a1d77e282",
      "a68288db4c934e5370d3015cda13de7bc835269f0b1153fbfee6cdcaf8d69657",
      "64be1c5242c55ba0269bebe4f706b7445b2e0a4710cd61475e931aff26833d80",
      "e70f9fbefc605a92f4ea76445a20a21007068babbd7b3d9fcd7fbf9ab5e7eb64",
      "1f42e3217e78f8a70670cde3d5a007884d678d0e6a0854aaa6d398d39622e161",
      "3dd77b4cb3877598fa9c55f4f78060bed546446ddc477517da4f8765aa14eef2",
      "85dbd5c6e1158dbb658fcd5904accce35ddc82015fb784d3ac0d7230bc968a3c",
      "098cc3e50a245f2b1de39105414d80015cf88678bdf44ed5dacdd04da0bb5b79",
      "074031a09fdff01740d7e86d0b8080487f3abc70dc346ae84e9badd32f0955c3",
      "9a8c81e27bb3675b9c74c1b22197e30e8f8f3ffa194270f690174da15e3e9c62",
      "262a0546ac4f01f5fea096b93b4f9fd3d353ef2f9238390bc123d87296050191",
      "ed2bdbfb28970b3e6d8c17223fa0df891ea275a75559a4810473290b9f3a44f7",
      "b2823563c09f5d7cff5539f7683af2062d9755c9cce38eb7b41c4952d0ab72c4",
      "4cf423dda20ed90ac08c0e1c39276f7f8b3ed9730ee58bffc11c309682d145c1",
      "3b0452c0b03b5b95cc30ed6e62e3169c153dcb069d52f31fb225257e62389a5e",
      "79c2b62aad5998052dab3ed63d497816bdc4599e3d6ed13a3ed509a70060e973",
      "4854e5b27191fa8ef079d28999a1d010a876bf2eeeaa1325f218aca825662943",
      "d14f154a4150210e07d125b51a9a22c99de9090c6176cf739619e7f7e0d29b1a",
      "b7f080e3e4bd9f025065143a68341a6b4da503018f7b98bbd7fd475dc3d626cb",
      "73c29f2ff5ffd034cf324094aab1cfa2da9bd8623f5fd99d06272703d4bd0857",
      "f499471e73e259e4d4f633d74659bc2bcf61c87687d42094c25ee7fdb353a93b",
      "78692764e5dd4095c5fbc496ba6ed2a4c052dd2787e8f65c3d1d9f51165f59f6",
      "af566e4294dffc45239a38b303e8045f91237981f629d8b2a92399e6220d52db",
      "f0eee2b9e2198c9938ca55a473b1a14f38c13e85e853e595763ebbe9679a8cb0",
      "3f99c4700874fbb997ae9f20d4c5d31259d844ac58047917a34283122b5235b1",
      "c1c4be743b6215dfe33c8f6f6c1f5fa57b404f4dde538feb026b3c0cd7c9c810",
      "906ed3e5eccbade5c770494048bcd0280b4426760401188609f1ffee91765de6",
      "68018f2f3eb5dfdaf9f4b94dd08f5f25377b1277d6ff19b1795647b8bee3975e",
      "315362684b13f1e3eb9e5b7aec31edd26c04235ec8aba2305dcc1fabfed051f4",
      "2a005ad928c4ca4069495ddb6c9e97ea0d5b7f9b2fbe42e848fca20d35c3b3ba",
      "43fb77f8123dfdb2d1df15c7d539aaee8707f0e3ddcde407ecbd26cf57d039da",
      "478eb06118193aeabc982182cee2b8a808418e92cb5344aa7f15e3120c6d3878",
      "c4ce3e1940f16c1873c3871f3e29793b9555f8e34abf9721fe1054d93fad1e15",
      "80bc1049df9a9a22b6c42018ae1081af7b2cbcd8885e5c951b804f350eda9ff7",
      "a0b07b6900c8c59d69ff7e3046e6c927a9f87955ef843ec555cefb974519f0c4",
      "bb6919651d17e8b2e3bce536d34de9c3bbdd932d8ca165ef2d4e834ff0979ae2",
      "3dc294047278c5eed3fda096c249dfa9a85b8b093661ec096572d188e8def52f",
      "be2a1d2cadeaded9aeb076efa0aac4a9e34fae6b254577edc3389083699c206e",
      "8c78733284ec87af4a67170257584d0f5bfa1205638868de123ecd89a4e17653",
      "acd6134e810262a1d4422307da22e344f088511765424efd7626cf78f566a8a9",
      "608ec26d293289a5ccfe6b9e4c0df08a160aa4701f2b64f110208d1700eb2a4e",
      "afce30534163031b6cacc17c06c10391b3cab6c1c023b443efbfa12e675a599a",
      "e6e20a7fac5cc3d5a28b9805d1e42283dfe3740a8aa0589789ab4015f8b31dad",
      "a68acb89b25d0dd19aa2a4faf7985839ae34847b19da6481f1562ac132af432b",
      "d4ec9ef099094989cf4bc556f4eed48f6619be34a2fffce579fb27f8cafc04b0",
      "df94fa375a3b5a894e8ee2ed9f51cf72a79f24e14a9f0c772a58d05e6e26ce5e",
      "d9989349c8ed910e83ff43fcef579e885a8056a0caab41fb9b98842a93eed3c9",
      "0565cf933caeb36f125f3004038e5e429f79dc819640dfa284498445d93e6225",
      "bd3de327a8e30aa0a756fa3f60015bca43298943af7b9133d798868f8cfda561",
      "960b8bb1264a9d5c946fcf0244791b18b531b0df94fd680d1d5b3876f8fa2877",
      "cff7b6ef6b1e67495fdf4d164b8ab19d705ef4b27b51701857292c42b145bb05",
      "e4cc2210a68d4b0a8dec54dc0a05a76d9bda91251bbcbe74f9f9c453fe849b7c",
      "9f6d2687392993dcb0780171325ec99c7d22bc8e6807812f3583180606a7edac",
      "506f506c6200bfe526442ed47a505f4bcbe99d896d5ea6c29e3b81036ff89345",
      "2651db9da5654d43aa4d3df635f9cccb862f700dafadf8d505795826db5ee89a",
      "4ea5591bed1b66eb81a38b5b0fc13fbc67f75cf0321a8ee9d069a01e09f78957",
      "4dda132cbc8c07f791adddb707e3fe7d4ee1d4865d929c47e22da6d75d420bd1",
      "4f5d3d0967c53631b7ee0f5f8aba068149c023a1da9c47495457c673db75bc71",
      "0b21764d15e687dbc33e63c38ca85ec82a840c5a44ce8ceb2996e1c50adbee6b",
      "034311689ecb99a0ad5f4bbb6f34fc4d3fb4735c409d2b1a835acb3e69decd8e",
      "c7540b8d2d35b43d49fe4cd1279922ca3f6f44e74f235247c49b764e4653c85a",
      "0d3120f64a6b7eb6283ff1ccdf72b766c04bb860e9d4d32cf9899be8810db812",
      "e9bd17cef428848f8549883294399782668192cfdd68b674786345eba858aba7",
      "0ec4161ff5f1244d8e170642d80729e2a4a4b69592a3e8e00d37b3a74b17a52a",
      "d2ec0efdd5b82cba217ae11e1c2ad7282e994ecdeed53cf5b9d635f04bb9e440",
      "5827346e1dde59f113ad382647f4254f8a4d6f32579bfb817a6eb968a4186295",
      "c52bd0cf5c5d71c0c75c0a6a1f730f8ddcd14b3522061817a921ba2015282646",
      "07479a7120aacde91fffd7c1450c86f8d69d3055d3ad08771251a93c65e188e8",
      "b26e0d1d0a5650c3454abd4381b860873c9cbe535dc3bfd7e2fda30d337e917e",
      "c170f0b57072ae0d45af92d50e78b2c5ae4dba51cfc7cbccfb598a5eb67f8310",
      "4847c322455b07dc23d9134085878c17fed9a429230e004fda88c89e8017ea33",
      "4504a347f8f0c135a8d09ec22c697eb6dcd122c5902c9096339b5065fd7efc7d",
      "9bdaf23df9e7681607e8942be624b8c241ca565f6838690f6b642e1dd9d8b774",
      "7cc19ae1af60a85e3afc2deeb84a75b1bf8d6cb0a4b61dab766796452cba2b12",
      "00a3d92cc9d15b30b1b8530ee9bb80d69d2afaa69d3b3efc4f84ad26bf902df0",
      "206fdc9d5dbd29601a68a263c16606be0e8b6ece29efe75734a31c200f178ff9",
      "1d2d2b55208630924d6b30bc36052e239e1d98927524216c6bb7f4f32e3dee6e",
      "6f22e68fb4ba74ca4c8965435ed7fd84dd1f959e7d7dfed77d07886fe9062f65",
      "17497943fe75eba03e85aeead6248a30508bf028092e1b5044b12c2a3911434c",
      "4b948d6a9fb8adeaeab32fc250daff9852a025be22af534eebdbab9ba36321fe",
      "2ba636ae5b0fbf47ec0e4068c924c112b9b96b89da292f8f66721694902595cf",
      "dd8e2e246027f699c33ccaa60e26225906c7e4b9641f19d93fce6f696b81c548",
      "4be1c313b387f1e16a3d0af5da9114c4fabea7ea3b7c12bf71337d94df1c31da",
      "6fbfdc4aafc8cce74029d954d5a8044069e591e74828902559d451cd8eafd9b4",
      "d8879542555665a1071a2f2bd4b26c5c43edd98b84924b657b68e908110623ba",
      "85abe8cc76375e594e1d21e95e70a6c437a167f561ebda3df213625c859be5f9",
      "b7ceccd0bb1efb08fceb2fbd6ee64363afaf0ef159ef97bb437b6d619c40538e",
      "288c9dcce4d094a41e24abce91bdf34de129fbc81123b1ad0ec692ed5c3783b4",
      "9f3b13485f3d484837438ed52261647249c0056134db4476229e6c0d0d58eb4b",
      "7d15e336bc330a1b4257c9db77577e22fef14f9635f8443dd02ec4d61ed5677f",
      "9a00ecf33e0ce622266ae93f67493f5274ed4e032f8eb068fe94fdfc60bcc957",
      "da8d2ce1cd4e299bc67270719eb98c1e4735a73af1be5c75a4f1c3caed150c33",
      "d1639a6c7b1a4c83f8dcbfb36ea7f33b31727f7962941b82773e564818ead7a6",
      "0c0cbbf7ae88568df6b120a8e1a0700d83998f315cd2ad2efdd41fbad1b79b7a",
      "b8152ac3f91d741ed628b8df4526e1a225df6fc800aa9b5d477f8948f8e3eeb1",
      "f72854eb6e01fcc6f76fca22d9c369ca5fcf64aae1c164bc3e8608333033626e",
      "f7abb5717b2bd6645f088366bd2381cf217aacc632ec0f2d10c351aee6360d19",
      "0f122ace2664b35f41c48be61252440ae60db0bf91467124c9cb05d21b0846e8",
      "53a5a9b91b8224cb4634edbfed7e7f1080c1111d06fd24b396f6f356d182b670",
      "2468e205f51354a15cde55fc8472111e12b9022500b48d031a3dacd73d637209",
      "89c43a2ae736c4178b47cee97a42011c85d286811c7816e6794bb92e08ea52d7",
      "ca5a23c03c2f5ff4b6d13e4dff02f6a7ade183165ce3f730a699e8f30953bc45",
      "890f65f2404493eaa5559fde745e20cd6422c6d8c02dc96dc66cc8805d7e4b3d",
      "2408a09ed6cfd8bdc4d6d8c7af9b2ed54ae8ff1c80cc47e237e484866b3fb2cb",
      "a716505e8a776e2a91c136d8d7496f4c96ccd27e0c46102624b64cdf135bb253",
      "641db520118fb868e332cdef526efcb4371ec937fcac8db3b318db46f6d0e150",
      "ec6ef9ed7193896f084fb83f098c73d5f4b76c4b31338c983de902bccee6cb50",
      "7d8b9fafb4591c714165e93b5e315c5ba7671a2cbbf903abb4e798ef6b8e05b4",
      "ff27005b309e365c652af8ad465315a846b534cb4be24fb4766e4a67592de280",
      "800634095d21b2b2b1dea2257a7907c4a318a3fad397ea1ab89c0cb0e7066995",
      "5b333f7cca00dbb5de2a814d5347a96c4894a001610b7d546e564a5415d69e54",
      "1e8ed2dd3295a8abb6140a669effa3ea954bbbf3f84fd5a4ceffe9de0125068d",
      "b3423dc4aa06e960ea9257479ef64d9807f3dff2e65ea78b71e0f49429ce82bd",
      "1fbef24cbcd97165639f7722168ededb8d5629a0a35f37837b6e305429a2c281",
      "c3237393d3a68350648afa0f431ba8ea1bca15f2df24e3d712e12f742c86ff64",
      "2e27e8fcc1d6809e7017dbbfdd72d5c5f4434e840fa27551b8173250fbc47d4e",
      "4180c48dd076d3d8758b3fdcff6cdca3abf122234d4bf26e5ad0bba48a64a78e",
      "52624f55b4e905abef683e4bbfe6b9a9f6e7a741df0e36fd8b279fec253fc0c5",
      "e3ad362b5df343321443b2b2b1930d2811cac01ea08f0fd4532f8b7727bfc730",
      "2b4dee04f33fd679b6d3113a4391f7e07262ae1ff69328d8cc22381dc998606c",
      "f68c1118dc9c6afdd1a1a2eb4f77dfef4654128285501e8058237c642130807f",
      "379862f743711118c52767028649aff88b5bf74e92a64f0f0feceb9a413fefe4",
      "4524a8a45198f6adf451ef1a864a00d6c834b8f8c48d3a3c1af1e3889580a7d4",
      "0bba370af7185ee78ca42e8eb68a4ba08e7b64c2a65758d6db4ae8aec9b8cc10",
      "21daf76a9d480e802ca952bbe1530e25262e36fcdf6c8d760b640489938b3f79",
      "a2b7aaa59d092ca60c0f73f776e4a9b665027c4bd074ad0f7d3f72eda7289904",
      "7a2bbd58cdec7eb32fd94873429fee4640e88b95c5d6a0eb6be206d3dabbcb6d",
      "f209f1299ec87e053d0459bf04469520c96530fa7aa497144b194a577a7fc636",
      "105593458c5bf00d4cb2b4f3f539f0be4dea99eb7a6c198a25f1cf3dae3fba9e",
      "b3b13548d83d957e72c19db9b978acf5cd1e47bb12f8b87b89570e45a0fd01a8",
      "fe0a047c9f97baedc8b0361af30da67885cfda9d126ec6470606e56ac4838593",
      "973bd5f19b700e5be47a8d82abe9bdd28620d5528c3ecb4459e62055562b8711",
      "dadbc2e8b48c6cf66cf4efd736b03203318482fbdfea46ef4addcf7228d15f05",
      "5f6f41d3957f8280a81caa7f31bbbf2ee8fb48d4812c1decf9fd5995ebf09559",
      "68979c49db88d44d54802ef6103de4f7c486498a243fdc645a902a1e0d34ca9e",
      "290d382a0cf7e30d959bea4e8f1a5f419e27040307743b883c72c1a81a3d5619",
      "6c0fdc1beed0e004a91757d9975070fb92ac897a15b053acdf38aa75a9d90ec7",
      "51a328df255e0ccb667930c32d67a8aae0ff04f6a52d2a32b94e75cf37aabb70",
      "3fd61bb76d94892a41ae3d6944a252a1e1abee80fc6e9300390f9e927b794abc",
      "cfa477defa2a99a64ea292eb0acfdd71440b52dfc08c9e8792a7fae92f5ed744",
      "9da8a7846643f2f4e8d4e0ebcecebbc274e103f8ddf6fa00693fc27fdd765ed7",
      "96831726dfec170f1eaf699820920ae606dbef1c3c8be1dcd9a406dc06813792",
      "aff94165e1b6bf3c273ea165e14c3e0d3635bf8534174cc58b21f4c08ccaed6c",
      "19078551019fe2a117a73203ecf3f09d9f80a61244a514c564100b981c5b9322",
      "c197d0b907df83e2dc78ec5813f3d74615c82fbbe06667e26bdeb198d58f28eb",
      "e4d20a53298ce912b930001aca77bdf12dcf697c14322281a1df14e204fc38ca",
      "5e635aaba55299caa809cf84cff142d6220d4da15ff5a593a25a546331511e6d",
      "498a98b6b038f3a9d242854dad8cf777fc742676cd8b18a3150f8a569fe2320f",
      "ca58a385b068c335a1654047d37620cd70f3a936f87770b17a4bfabd0afdd89e",
      "f4586807c3e87122968e402b742faa4572dff30f62a6e45d66955cb264ac2d0e",
      "8ec4edd0974c431134dbfcf9ad00fb583ba27f535f6d6eba3257b2598561850e",
      "5c90fa5031ffdc663ee4ec6dae024ebde48bb3a5f14d34ee3068c83ec3ce160a",
      "45321107c68c0ff2184544ffacfcfc9db6322236e61e66c0715f84cb15f73bf8",
      "6de77b147a7c49feb29a871e3e075bdb94c34e4550129ff8c77d4574583fe49d",
      "6939f4c3f13436a6947af7d922e3ea6235837d057012f23f09088e4306f2124d",
      "00dba1ab2139731a3c395d346189dc7f63055d6183a1b11663056ca673518944",
      "e748507018afdd3d15a15b27f0cc3cb4703057815d633e49a7a0b42b2f763be7",
      "925501962ab430e7828fdb89db79699e4e396e45f8fb69aac4fb9f2adf3d46f5",
      "79b74ea291fd531892431771bd261f1a309553d6ebc4d4e6eadb1c336586b8bd",
      "9a02bc16e22869cba716d23682e2e966097d21009f256edba41a75d964e196e1",
      "d4c0bf787e10f938104b21657f5827e4492ad7de656a417cba7196c714d93346",
      "ef682e86892e79c91944c62759939e36787236e57718b6e1c465494ac9c18706",
      "3f05c1d40edd432a401c42f2ff8cbf7fd22246a3ceae7bb98461dfd0a454d80a",
      "e2c5910929336d4148c5eadd0fa580867197c20735f1a58583813b093ab6e7b4",
      "09c64112766ba62385007a9a2274088aa9b2e8fe11baef53dcc9fa1f08ee9055",
      "b48424caeaffdec8cd0a14bbd327ba4e8e8b4f31cf41b1dad154c0d3a5172f8f",
      "eca611297615feeea11e917bfbf4c8ba3d0045622912d351f9b5a4d08a298958",
      "fb22958cbe279ce9ae3ce6a1470f993da2e73c56e800aae6ed4c95b829d8b19f",
      "401a4ea9db588e88885c7cc717164591b30a939199548fba636b3d10fa4f2226",
      "ebd7e29c7dfcd196428e8363f217a97d821e7bc38df76814125bc1e426f9f4d8",
      "84fa2473d08ca5aaa5c9dde6dbaf2b478ba8cebc06f763c74250279cfb2f6b14",
      "91d0730836936dda0b79ef33799e25d1e361d5a97ec94e25afabdb2fce36a3b5",
      "207164c6872d131f0050e618a38ffb991dd584da0cbe61e6f334b688013bedfd",
      "ec44eece311122bc5c848aefcaf66aa62ea5700e2e4fc2ace1714732bdeeffb7",
      "7f999827e5befd468e45e95e2b8a837ca5e8734f28a9e86853c14d738749bed5",
      "1eb73d1c732bd9d65d120517ef82f0da445d177d1d4b4c28d091e946419c1152",
      "cbec8143f2232b77e63c7fd5892153b53929c74559a9302a50fc43bb55f4b6b1",
      "90663d94299409f2c649bc5f8cfb98ad4116b266737c5fc92372a1728d5bb940",
      "398a22864bdcd66fb9daf974c235109ef65ca4339d3319c5e2f8550011183bb9",
      "8a4360ff5562a6c41320e9b407f8b6967aa81e63ce8e1da8d29ac7617abbb8f7",
      "7c48a9d5f11d7c85d5e008df57b80eef52461c57b919c8df74c9b7c9de4a7ff0",
      "2ea8f5b84c388621dd620a4bf3503807d3bc85e42bb60246a84a658c2686d00f",
      "d4a20b19a0fc482703674fd8cdd61a53f836cb6f249729d44b055143fb7b0a3d",
      "16a3eb80ac299f9d083199add188f42cced823012cb7c073615496dfc82932a2",
      "ebaccea5751744dc7de0a999531922bc9ea3c21227bc9b3b7ad57282dce16a03",
      "6383c06b6fdb530a4f126f4d30c93a9ebe5adcc38edba08c721e93c809586558",
      "c0cfad163a688e20cf66690bca5a343c4e0a29267b315c0d3eb5214bddf3f863",
      "aa72c458dd5867d9adaed6f212c80818d2301c639504dc29b75af9d40e18eca7",
      "254ee3304348df6ff42b2b0431cc2058e8366581d225e34e92b1153dbd918dfc",
      "c82a4788bdfbd79dcc17f141dd5a26bd44106c4b71513767187144a464fb56ba",
      "724ea4b2210ff132f842726f13553a73cb2321d4530ddc6c8d6c45c7a4ce517e",
      "2cb16b16d9bf5d6ab174f28ed6eb734670a075f250a3d3a15d8f60203c49238a",
      "d1238288dce3d766eba77840688e9619ec87659030355172bcf90c88bbe38f04",
      "a2ac11bfd88dafacb449b8a1b7dc74766ab956d39de938a410c7fa59db8149a6",
      "329e01b1444ab12b0d33e8730e95c94e0051da29ec32bcbab4282dfaebf3f9f8",
      "28ea70af19921dd0e19ae4c2f551422595c057931e5a1d2cbc03cbef1da64965",
      "9efb7cc049fe0920278aec87a7eb9a6388b4960f94de904cf3345d8aa6625e3c",
      "3af1cd30d8bd9b41533acb4ffb32042c49827623f861729e9bb96ef6be89f980",
      "6b4c5a5d7832fdeef0dbb482a8f57051255dc382a3fb3486548710e95ac8f4c2",
      "4f2cd0e3352dbc73a43b5713b561c40aa42a62945f5466243ff50589d9031f5a",
      "1e8b8361e77904eb085615bc50f8ec63bec470dc0925a7a905aac5e72043b71b",
      "da827d04aa8728a4573b547c5320d1c0e1e2270e39add06e234c33ff0fcfac5b",
      "8192a683f7ba85f2d42a2babc785cd5ef7015bee4dc8b62234a16227d59020ee",
      "8308e0b6530b83e2bef016eb9231ed0f4c80c90fe02d13cf4c7a941cff1248b8",
      "3a8d9abc04210ea8081fe412bd710fb6c1075c203fa92d353ed08e5f558c3c9a",
      "8744af016eaa05fd05aea3f946ff1f129f976ab6eed9324719a0cc8e55b40525",
      "961ddb1bcf4785e13d810136564b94dc11bbc76655f43965c7433e9c56b70122",
      "c5f8ba76bc388b5f7127e83be6e45f19f938c9036c191f9fa900d7a1216b5167",
      "b906e4ff46cd1a61759684ab660c08b659bb8902d15bda29d21baf1b6d8f8f7c",
      "88502fdb9b9b2b6de6189847734074299f664af352f946d6ea6ae6bdbf66b9ba",
      "431b89ce740ad5fede6ed0619eae247ba4368c22dca72e78b6c7725bb03e86d3",
      "13669942f13eec7ffdc249f1c68da6be022e49ce9a5232cd45e2675e7eec170c",
      "12792a4ceaf568ca5bb891f9cb7be9d3459de5329b775332e33a072b0531cddb",
      "5fb3b8261f7863d18ba04d6574ef6e7f16acfdc9a6b86b7886cc507391c550b8",
      "74d06fdfb09593bf21eb4766e5364adcc368a3bfd2e4ebbdbb49f09ede359b62",
      "f8a47464520282d521f671576bab4e7beaaa5062db0fa7e3f9b3dd01032fca70",
      "ea34c06a4a0245339855b3fa9134016acdd60698f5a730328831102183ed4daf",
      "55420199e91ed1a5b5c7f61a82104ba53641058799c4043180cb67fb52dfa344",
      "c907fbf7321b8cce91d018fce10b8823919503e309a3d9a5dcded858667fedb2",
      "64e8ac5e6017ae0083c6410cadfa623a6c8c0071450f77451da3c446da13e766",
      "5ce546f315bfbed0c2f3ac73afde8a8fb221439ee7db7029a51a0af67b4eb774",
      "c78ae513a7679488fdff070cf5f1e69adb0e6fdce534967a99d7181419140f23",
      "47350454a54c154b787f0ffc7cd9c1220249ddd9a1f1f6740e113c2669198878",
      "fdc46ed6b430fc91c1a9e04ad29e65fb9b5622c0c22518fc80f961a4a075350e",
      "9f76c12359b60e1c2efd44fa926b1126bda754450ffa570001f08fffb0f7d0c4",
      "52b4ec69195dc7d1d1e0792a769e147f9a5c4787ad6fe1aeebc0a3b87ed12126",
      "e16eb2655bafc120046f67d284bd35a54f589ae16a0fbec62eab4335873db05c",
      "07628f49399f93267fbb86409521a17b347398b7d03442400d15482c52f66eba",
      "8491853544cd25a2117d5cb94f1e7c65ab5a252d7749ca6a939d5da8c23bb967",
      "37c57cd273c7f6c57cd79d8bc922c4ea51697ae057641a5c26efa1cedddde71f",
      "bf8a2cda51bc60e4a5400d8d8e3267677ca1473f2350656d1652961ea9e0c7d2",
      "e406e803bf0370e1c6a8e8d3918de271f1ea02df79afceeeb9700f7481a99fa4",
      "d4642298119e03236c5d96bdb2242651b76f161866fa9b38b2e548c5270ea568",
      "3aa80e69a239566a4744010e6e94fe744d153c0855b7abb2177aed2c972396b8",
      "45ec3aa2a67c4b60ce554c1f3c652471bf075a9adef2a64d100c04746cb9e23a",
      "903df79c671fe590e87faf559df2a4f4535d3405a14f36f7f962332125899be2",
      "c5eeb9acf6dcfcfa7a4880ddf1fa2f2a9f7721149cdd3f81cf9700e0c90c4b2d",
      "6f0cc0c7337ce56e96fe8cdfb1049045683b2f1254c411c45556a687110e75da",
      "84df5b360ac3d4a0e6c056050a63480fe8e4a6c9ec6729206a29463435dcaad7",
      "794aae95b2bab54ab23e677eead607df97cf4c3d1a35535eb626b3b6889a8a03",
      "780bdf009697ca54bc02a65c6680457be141858dc2c6793b8dd3ed962019c394",
      "9f433c08561bf49c37167668999d97f29d42ed8847af1f66a23681fdaf4d29e7",
      "0036cc4d5e01a56741fe521f91c801dfd22a3e34d5973afeab4dab9aa8f7809b",
      "23c3e13700b56433ab82e7f075d870b1d23f99b081cfd491d5b753c311274f68",
      "6c9c3b795169e38d45d9113cd8d0ce0b506ff0cf92ae9d57c8f91bc716d5c176",
      "1eec903606ea711fc17c7e7f16945438070c30f95ae36e607943f1695fa9f245",
      "42fef399534f8874d0f90f99dacb83488e22a15cda2b4e0cd296d2a4ad3dffdc",
      "b4f40c6978a23a83d88dcf5960bb5ce721189f02d961b12669b5fd853efeea24",
      "ecb878b121bd74080dfe5f9fa59d7e6363376db03542a1bab57487e4887784f1",
      "3d70edbc73f1c812d003dffcd1d9c1a77c1c8d48cbab7c0f0a65e726f2099e79",
      "29694248b1fa5b720f4f31671d63718fb4435e06f530296f841715a338a8bd89",
      "43fa0825d5dc86e534d8745581568728cf894d61aa892f4d1ed23fadfdf7d1de",
      "01505898b80d59a7a262bdcb786d8b992428308204b36f770b4b45b58853be61",
      "3e45417507402a30b767c95a3212424b8a38cb6c5e400ae76a40bcdfaa8021a7",
      "659d5ac2b35f868fceb33cd24d825ca8f8f6b5f3f45ed478db5b4317a5818a44",
      "4cb60cb04c2a2ef53a2e3f0ff007382f2c8d4b74dfd745e0527284f57c90c9aa",
      "d69fbd9c019b1f5c0bf42b50e5864568990f5dd698e3460e6a5e331acfeaf840",
      "13fe4fbac7f5103be178647635284c2c156dbb6e74f5966a65539e4b47b09cfb",
      "8d8c46e71864972ec619af6964e808d6f99f39f335b90c8b162e9a2e3b62eed0",
      "050834c9b3aa41ab1b4b24fa95d52e7d6d3e43bd70e29fd70c1c6682ad4238dc",
      "6187aed74e90fcc0fd9d49b9718054b4ab346dc744b510e50e998dea85e85a91",
      "7b7bcfab9c00112fe51b7bb6671652cb698f2c48cceba7f74d00c7682b7dd3b7",
      "f185881c9d72f2a86cfba021ee700c91e7c77ce8ae1280c91e32477b69b6a909",
      "3ac29d54c36335b3dc090c8e0edc445a5366c4d50a7d02a1900ef46f37016202",
      "076190f9a5af07f76f2e097ea19431417501ddc391bd0a884488ef5affa18277",
      "487ab89f25e9f8bad755de6e5dfa221722040b866712bf945698fe813297fcd3",
      "568b60c005245a40035e25872f38c6b5a8d69581ff4c1a19838a9a04396fad38",
      "2cb92a5b9bdb6aac3ad6eeb7f74c3913e0db9abba23c9c4c2d9a651f635225cc",
      "cce4bf53c46654d6ec626638f40755a998eaed1507c7be27541a265d7e593059",
      "f8b248a0b8402c08051183d627915ccdf753641331e87af3f70cc243a4667035",
      "e9ec9eecac1a637acdafa464ed35663b8e46ed79475a6c10b3ae616e42991cc5",
      "7fcc4ef838869e3a5b342774f4376ba9000918f4bd52b913ec214db5b5192989",
      "cb3f51d2b185220f84aac8ef9a3dcb14bbbdbfe357186ca599b6b6aec1cf8d21",
      "6fe55a221e731640f701dffa86f94cb4168d7e561473982d42c21000ea9ce7bf",
      "ba9574c9ba3c3b81388fa0f22c4798f9f0ea5225d5f4b23a3d68336b30251966",
      "252ad97b7d16e7c00ea8a82c1997e40313982cb135d9cddda091484f9cba6a6c",
      "e70f1592b16e29fba889a3f64b30a79d485b416608638c26742fc999c65c5dcc",
      "71882fe64fce1aa68842af4c1d16a30aced7828de5d58122a7a753364bd11bec",
      "fe0b1a6fe648e1a9325614e790bbde1ea6f5139b4d04712f577857768258bfd6",
      "5bb641d75c7ee48dc32772fd4177ac76c56dd41ed5580dbc0855efefb16ddb04",
      "14b3ac218836f947a605346555cbba3b8ebc994b41eeef85987d54fc0f6b63ef",
      "4f88c59eef59c41fae6349c00d6916800c5046a804b370fe7581e3756cf24546",
      "1e3a14b22c699aac8b94a60f8b33608f3041c8b39841d04a513fd178d6e7b935",
      "7a0fef52ac903ada5ea5cd19dacc0a7245dea6d7ef8378b7a8cfa4f630a3d0ad",
      "2eadebea2c1a2c242db8ca6ba6469930dafb8873e720b058f90691fd7235c6ac",
      "2129c3a2ce8a0c10bccbc114c78590dc93969036c3147ee15bd87f17538583ac",
      "eec30dc64584ab4bfdadbbeb756cc91f0dddffcbc406326da5529d36d166d1d3",
      "3990d5ddc5eb758c9d74981d7ad6fe00d483f925e58fbccb9925be272eaf7843",
      "8d8052da2ff54a3dacbedf7f22388379c253e0dad5235d7c6482a68cfca1bf64",
      "888cf3e86687ede47ea7ade720623bb8adde7a63791d3b3d78c3580e6372d7c1",
      "abbd132e1ff81e3052b63bce70a509b564e8d6bc48d611ac8a7fbbc7fb108892",
      "8d05b99908e50b38eee6768f3ec2a23393f7c4df8e01d3997bb425086ace254a",
      "b0cc6ad2e36eecbe7ef77330ef9caf99d48e01e369069da8e53f70649c5d3921",
      "adef257a59c3e0feed8cdefd00bc37947c25569bbe51246177b3a368dbec3e2c",
      "dbcbe9722aa146519ee0986350cca803d1923e067a366286ed94f958247395ba",
      "f41e6add56bc5fc7c18ffcb33d02d593b640d078ab3b602dfb83e502c1650d69",
      "bf6b087766fdc9eb4aa4674a53865ee9488fd1f430a239f84a05267ccdcd6b36",
      "9610e437fe16a1530fd71f3cb20926def29508ebc50c586bd9246518f0a5e670",
      "bcea7017daa535b2c9757a51c8eb4ad6ecd5a6d7c866cc4c21a0a5228fc2ff6c",
      "54ac50501b01a89c6f13c5391246733daa82cc21213b04251c4a25d3c176deb4",
      "b72df1c11ddaba703af532eaca608275564a22cd76048f3d646b48033e575a9c",
      "f5d4a5ab3e22547231a7c5ffef9a870b6016592901b6c8e5dbfb6817767b3aa6",
      "eb431514162349133db826f15c952c0900cbe09463fb1247ce28e8092d873264",
      "3a5ec2d1eecc67d54cc1e019df001bb10d6e87a808601ded820a4c9dc65a39ad",
      "ef64b1e440b8e66e7cebae4e866bc4bba620a1f52f05146abab7bdb1cb130162",
      "839a4836227e546b636214214736e3ded88b3b139bb39e2bf6c65f5431a8fbb5",
      "8e080161c79350aee8ac2a76c43d180b98087ef1632d536256ab84e5ba37552f",
      "6397d599592e760cc9a43b09bd98e8b3a677217332723423090622f40bf148f9",
      "3c89ce995b1c354b4a7ff5ec0c7533c00dc10c79a8eceba352d682c8e595a830",
      "b9ff89d3928c367f37e213a0b4f044f748000faf307e7be9d0d39f33f0df5b4f",
      "e38edbf5f82e1cb78ecf4a9322afd8d0629923857a5fd485eace3c27937cea3c",
      "fb0e614017bc668bce3bbcbdfc75a1daf920308b5adfac2057dfad9fb2b36f44",
      "a3d0a560e25e25f79e8b5cfc285738fa1542717c9702b2069dd3aef893f72d43",
      "cb489df68dae057ca9d4fd4645bdaf54304e5e6c58db1d4c686df92cf4b9d641",
      "f15d331bc6b86661879702552bd0694c0d369fd53b2bd2c3dab2c930c7891619",
      "24c479b1c7c3977d78f1f6f12c6c52a80e317c804345c1c124bdd9e0441f3965",
      "13c1c15589981724ad2ba09967f7eec5dc98e7bf51ce28a49fdb7a3f2b17cdb7",
      "68435c6d45bf63b338d0bb673e11e7a298d898a269b80a8f53a0d50be4c3a64e",
      "cb9bb3429d6c01ca62dd3cd4a8d1475548cbcb71d99cf9c1121299e333844372",
      "e6a6fc405aaab56b3d32a896721fa6b36c6febd63aaaa0469b15771688c981df",
      "e4e887380cceb32850293bd587b359976871d01e58733297ac537b2d83e1b90c",
      "de2190f556d79a1ef745d370ed56b040bde708f86623ce1993f01d863f4a40ce",
      "f00fbddde4d9331b74793322205a897493b0b4676355096c4f5402ddc6fbb3af",
      "1d89f61b4a41a87fa86c0f791b0730f944deb75c38d1e309bbd55072ef23c6a7",
      "80be7e2ba745fc88658d0394b4840ab93bb9acedf9d4a7bdedf8b98eb5c0f8e4",
      "73a4aa4e37d5cfacbdf58051be941785be743b7bec1d8899c9c28b583b67c5d1",
      "7006598948f349d86c114de6e855c3f758c16609c1ea73d09feb8df9378da860",
      "4978abc33ffff448cb8b26d737b325e84f163f4c35c6cc8800383b2783075779",
      "72f43e74b67643d2ee8ff76b81fc2b47c3b46effc83f9766637785939f4ea61f",
      "567dd62884380d1c6e156d8880d1e66e130787c8e271bb60ccf949f59792b1a0",
      "c665d115d72861ce56facff184018e1329098c421b1629d0572a5cea72ea9dc6",
      "24c905e6e8d6268ad553b982cfef44f7e34e4d543296a00fa4288625e09ce7fc",
      "d812c0608b2d89b6ceb75580f8e11cf623a61b6d76546b4691b7f0d46abb766d",
      "a44fee9a47cab7d385d9ab4af23451aab4a0104932344545d8d554cd6d2e210d",
      "579f4a888ff4b444176f70f2393635e2a74e765cce1e8a2b677fd20d1faea359",
      "ecfb327e602013bdfa2b4fddfe3d75c13b9e772ca37cee766957c32ef7db3d37",
      "86444e054dbfd6425b6c4b90d675b6b4826005aaeb14e233e21753f97165620a",
      "70ffa2d38d8017477268022770a1af817ca3b80c0719a2f3d42cc7a1e9c12100",
      "a1001f969b82379b3a50c87183e9e5fc93c3917e92f0125f7b751caeddf766b1",
      "9c5b0744e14cbf3e5f56ae2728083ddbb2af834a371cc5483996f077865c5625",
      "6f2b68d00cdb0b97943e266c2733c4ed975700ec51feaf5778b5729b810b9f16",
      "6e92fc1aba30fd11e737703df2e29daa723ecb6b4ef4166b6aa7eacfd35a7fb1",
      "91f458fa4c70f85043b2c5e831c318d8856741028e7239d82df9f32f99113a3f",
      "a391dff0c23a0b24b7462f3d2f807b4edb5dcb83b36d9d48c1b943cfe8662166",
      "a8a1dc7e9a57c24320b4be8aee2f2648c3cf4a61683631f3db1f8d8cf045634b",
      "3d1d66d0a7a3f46926314d97598c5756bd1950d5cb6dc4ce55582509fb4b6eee",
      "e81a95e0edf4724b998211d29baba19708267b9dbec9ff21282387c0287f507b",
      "bf95546585c760677ce8b03541c3bd7c788c6eba40fa8bbbd411cedc9a71a8dc",
      "1965e623dd218870672264fd0ae40c73ad4b451bdbc709a4666af38f7e132436",
      "4e36cc650c3f32b4b08785a2c51a55bba411300c367c09082d7f73491f5c4986",
      "c138db8af24a0b5f1d94502d2310b0f1b51fefe513734a79e12b670dadafdaf2",
      "f466ee10b860478f645631835fc0fc2313ca89fc66fd830bef7ae64bea8299ac",
      "ee7c99031483f0124085f1e50dfa1858dab0aa75fd8d666c54f6acc3b0157358",
      "a0f12c303d43414bb31afdf12e22dfcc4dad4fd90c221cdb64f87e8252a70804",
      "d96b15f9317d3d75ebe7d6f01a98b55f712760d46f729eecde9c3c6686463161",
      "2a3ffdd2fbf311e9a3b3e32540cb320a289ae01eb01ce58e63f991b40c305352",
      "f24acb6af611a1032209056fd8e0bf4a154e167cc4624190de19768979d04294",
      "56ca7e6ef42edd659c8b91e5072cd5322ada41331a314324ee5a635fb4309c1b",
      "0f9cd34e73a12bbe8ec9b61602276cb7e495d8a1d6f1d01657e8f46defd8d5b4",
      "dd331c149cd14aab7685cac2c5f7867898d9a2438d708ff871eb3a1173d25e82",
      "82c74cea2bfe4a093e361d32cce4b741d766dff7462d99abb17522f78f16901b",
      "1fb05ab0c9593b4a8bc65ef6c18f970c0e31fcc24e27a5415d6b9bda2af841b1",
      "f8dba8e510f3615696689852c0cdd6f5537fa6420ef97016744648878bb61a16",
      "31c5f9c05439474ece5c13e6c48654976a6593ea0df86df7e98562303cbdb52c",
      "026b2dfb1d25913d2fded4bd9a0c2443203e31cc5cc23029b9ab585e388c20bb",
      "7e20aa0c8d09f91f19bb24fc3ba081511d8c9778cf7fb0149d86d96735ed7f50",
      "92380ec8dd944c008e7e2342f14eb319593c2ced4510686e2ad724e42a04981f",
      "d90895adbaefa8d56dbc008f9c42f020799c6ef32b2ec0107d80b168e01f964b",
      "8d1fe651d740ed9db88bc2580f3eef4eaa3476787a2e9c6d60405f36f9cdd821",
      "b25b39421dbbeda6526cf2a47411ca4cd2ce064313b10ff9e90f1c3a819e5788",
      "ed0f819c2c44a9aea6d9d31592d2b0599de7d26096b43d4dae0150f793a29182",
      "76712ee076790daacadb9e97f9bfa378cc2ed2e5261559f27db40dfa48981fb0",
      "4c7bc581075fc222c2f6e22f39c725290f0318aea3b7c4f4305d2ca337dba34d",
      "ed9ed431c1394b20b22a5ced28cb746fc50af8512dac5aea04345550105d4cfb",
      "f1fee5bf3769ea11a88bf421074c3e1a6b3e86e8dcaea39bb1902f63566f9260",
      "6d5c1b3a3d5cb97985a4a2ca9dbd52749a589047b8e90fd759f80205b03e1a46",
      "a8f3944da85e1d2e1e464e37bc78e27f47343c8b2f53fda93314a5c6502a072c",
      "c8c67a323ffcbae924aa691312ceb1de4e32b6d0eb91918ef90b108b4ac1db6a",
      "95ec3b782e3158510d45e2baff18c290beef7bc1dd68fc1e6640216b6ef0d206",
      "c8f324fceca9a0a9ecd7edc5f7c2a334f6bb614dbc6114b64a887a7805629d00",
      "36adb90fe414740e7f1b4c9d18afb96fbdd38393a5dfa5aa9e7cb3803b62195e",
      "87be784eb1d317a56b0cbaf60c68fa557f590ddc5a7f4192676e9c1137b70e4d",
      "d33e3aa4eb6f45aadc63118047c697fbb80e628f517b644747a16694d51fa6f0",
      "65adc539b9a94bed28be4c92c20a2cb71b9fd53ad70628c6c39032955f4dd045",
      "756fa93a79adabc873bbc54f27ab97de74a6de74c4d33150322169a742e896b8",
      "aab5babe616ba0c0eae0a0e643d82761fc9ccdd57684d067734dca62c97a0c56",
      "fb520fbc3e2598af6bcd48502933b58a3318917b21b19f6c540d0fe5e7d05c9e",
      "7aad5ab518739a21d3529ed0e23b2eeaf69c6f22597c576f6911dacf7ce47b54",
      "b8c03089ca20dd661c7b02ed016afc852e2f4f04b15827d1303b1b65b77537e3",
      "b5401f671c5f6a9d2fec835d997949a86c3eeff0d9415e5755f487042e507b5a",
      "aba987ce6c582d25655af6a78ee04fd986ce2f6113f990ea91236b93ea51fe6b",
      "b546080f8d42b65adeb4ab7724750547304237e6fe04fa312cdf71c8f5e13dad",
      "ec8d8c60fb796d65b2b31bbd527513c6372311444c665bd17b6a676373455453",
      "c23c5978ca0859e44505481b6adc8cba428d219cc1b186a04ad1f45c9840be9e",
      "719d23b9fd5f85cc62ccd7956d7a907b6f27377c4f002b3686cc2446fa28ea61",
      "9751717caa179988160a8145677847999ad1a3d8564ed6f57f602ea89a779b57",
      "d5268100172325e89e06b587ba49bec90d87627c103cd3aa8aa20403e429ad22",
      "c7d4798491248b768ea0b5dc9c310597225f12c961afd3f395b6df3d21113ef1",
      "b9bec507e69d0aa8b4943840cbba5f3491cdebf2936d1201f9d47ef89ed283b0",
      "ebd371508412608b1083198886d1a406391aec8b371875c0252016462513abd2",
      "056a0b3358fe4a7c737f64904b0ffabc0cfda21050e0f434269bfff11ca574e9",
      "ca4d1ba41eb6e7cc59ec346a7384818b35e9bd7ee773b17cb00103c704cfe1ef",
      "63711b6ccbea028d649d7d7f3f526c7d41628b05ed6c5ab4c62c997a83369af7",
      "831c2c77f7682338797c89bee65787972617aa012bc5c58a5af625233a350a33",
      "71b523ec1cc802063f33c9d39506a2eaf6f6e1d017f3a1b08b4b5543e826113f",
      "d34b583f57bae7531541e2893450dbaa9a8ff7be327a65e342bf3ee9973b9a09",
      "6d783a8b0153ae2e884f021686b05a1009c0157d76356a97428d02c557c10f71",
      "70f341ef780ce5feea87076be3d1363d532834541709ddd53dd3af4fb6e9eee6",
      "066fd27a1f5d187233d051e9668ecffe24d70da149e1330e11bfa593befa9985",
      "fc9e3fa502c71b737829a0e6cac5f2cd4b0580e0e3764a40dd54b33604398e92",
      "b0e5cec9080a2859744a2574a708e51ac4f1aff01700b81dc596dea567a2a84a",
      "3ce5338dc3ca382bf3746b11da26cb311f49646dc7b5b3a06798d4d0659dac3b",
      "8224429923447e3c9159fd9cfedd8a0645010e8592589afc736041fd74f41ce8",
      "1040013cb514244227018ca1ced6bcfe0280c2fd5887567cd266a3b2815371e7",
      "e24b774f02a1bcb295a242f9959d2bb4d35d49144ba2c914b7295f73160f05c5",
      "86d4a3a66aeffa4ed61fd3a12e68d277ff61277d4f3ccf77329b480479bd7c4f",
      "af7ac7d4f3f87588d5e2bde7e320ae2fa1a15979b67a72202ce768bb603a7742",
      "988b8ec057a213a5ad1a87632c7033e3e01138bec635d347f1f216266544489f",
      "bd082e2da71d5cf2f064942f8fd1fa0d37ee9c54243154bf1818d0454752686a",
      "22c3611bc09770babc79bf58349c553f7f12813ca079532ecccd5d5179643985",
      "1069e12bf9bc31b10f4c9a3307aa86ea93891429f1722a142b104355a707b5fe",
      "e13a4d1d34112b00edef5875d1f312a1c53c9224bbcc1237114bbd27a98f3263",
      "ce526df7fd5cf7c3ccbf780191911ee4087ca72796871ddc36c0f15f19ccbc21",
      "6463c6f14fe469ffb5c7e7520d5bb141592e94d5bfbb84e14bd4f59162e345f2",
      "79dddf7c78afb33079990cca351f871c8ecc6104216b4af11ec09d93ff797fde",
      "75a5d8cecc8401f29606fae10fac39d24e37a9ae6f6148d7f624c5866dbc82d1",
      "ffe6c9c50e4b12a2b86957fccae1b40c325ff6369ac6f4e6baa628a2cbd4beaa",
      "fd2c70e359f51425cafc10bd0fcf8485a440ac1f85c6f894052d7570fb8f31c3",
      "e1341e1c8a8217c58d082cf5249f03d3443d487bb94896ff7e61af532b1d353d",
      "fe1698c32fa141e492b3ca81a8ecfc4777ddb77fc29d8c1626ca0cad955deb0b",
      "e9c5d24d4db4e6f7de69b95ca8813bbb8d68196e7cb00d3f388bc7a2bbd155de",
      "8a1248cecb10f66557c59cda8b6a962f981392d685d5db0eb9ed07bf2fa48b43",
      "b540110b5354001280e23628559d497eddaf654137d744f8e3b05100ecdb90b1",
      "4a1e6dfda996549f13bf5bb05da2f2a9839f9109cea5a4cb3e788b5567c376cf",
      "e90b0a8fba5a4baa0529e33333be4937e1e025fbeb3326e8b850aeb2c407edb3",
      "2e98887bf01db6badb28c8a411e03a65702db8a985a53178c2a485e14e0763ee",
      "49d747572c766ef9caad8f1690f74e2bca6db26974701f46259360820ac5d682",
      "c38caa48267febbd1a53364e7894ec5b4e695338b88601ec4f433b2c14f5efcc",
      "872d57e7647ab4d640715fe7dbb7b927e2edb5e8bfd6586bfc345f157ebae85a",
      "6ac7d1af01a00e4a75e3843da0fd7441585dbee09836a6d61a0452459da4aa77",
      "5cda1f4f51f2a8ef18ec7ac0afebdb6134d28df3c8e30f039c55d5de91a88b8e",
      "fc1e2cac074b21bb33ab86242afaf807cc3ef155834d3e2bcb50656771de0777",
      "b2a939a317b8383faf12a605e0b0533f9bdabfc240a10412975d948c2b01614c",
      "b622c342d1f33d911f7d6bbdcbb1307c96c84f652c1f0323fc645fd5b11340d7",
      "a556c2cc3ad92a6fa6b4fd556256f2e5ca174ed4d79d5d4fc194e7b36ec3d21f",
      "5c50d7894d83c28cc64ed5f3392a7e54cc81e853d7dd4049d792c77e9bc852d8",
      "212a2d4dccae237ab40c7deb0b97b0fcded4dbfc8405dc9d50674359336e50d8",
      "eaed188091bbb2ad69c709b54e8b10bf45d5b1d9305c95afd1a291e05d525092",
      "53c41b52f224b5bcea93ca664ac1675a5d8e4f11a26336bccd3c66a6933eabc3",
      "255bb6522e668438e4ff7c3cd6519ea5c9ca5916d2af96c2f180921c55de4c3a",
      "360bcace1a1fac7e46bdc9032e209159fa2ef657ff884d665f51cc6d7684c0fa",
      "434eb82f699f12784f2d5e84c52de299c65b2698cff7e4c7129b827ac6ee3d48",
      "c0d39bdfab4bfc2897a8c0a62c0d34576608e84ec3de20cad96cce356d36d432",
      "fad5c1c7b86ea3ae6e7672e940ddaa8258d8cebc5767454f4d8cb1e3cd9eb110",
      "79e8fdca27540d0256606c1c587b63e5ce31cb6e0cbe1cea4dc2d00c0089afaa",
      "e8bedf95be822c8e8e9b3624d371cca7a8031f20e5b2bc4c2919e1093d27c38f",
      "ebde2d65bfa2e800389f5d359c6f0abde71edfa5778978371d165e4a0a6582e0",
      "c9cb640292d49397e3ba5328944d4938223b7ba69205a8230915ee4767d75f8e",
      "c598dfaa35807303284e8d785e165dd0704ae69dbf45c91940010da427c5b5f9",
      "1421a19c481b2de1c07b4d78671a40c41c3831fdc409a003db8d031c3f30a922",
      "fdfd43060893eda51e9ad047d4f1f3b6201850fd7150fb10fdd84eea4e2ebeca",
      "f973142424053de744d9864b568c4baca7ac161fc8710b0873ec45b629921928",
      "bb1b29cfe7dc46b0924153d73964a933e85b3407740eb248703b7e3dd7b11d1c",
      "8c5a7a9a453c6bb41fcbc8394629c738a908bb0787a65b47f809e4e5d9ae3fc1",
      "16c9d6ef4ca3b2298becd3286e7754b7716b11f6db0668b9875e6723ba76bc9d",
      "af7e09a74df3fb84daf9077b23c2fc099585772723d01a37e96d9ac66efc904f",
      "1a76df0249551ffcf9ebaf67cf731a73b9e755af2e7659cd52b4c8ff24e1138a",
      "91e7a8cd01343f7ac68ff73af683db5c301efeeb4ddbc2262473e0d6579d1ed2",
      "4cf95a32a2900269386bb9484dcffdffb7a1ba844f161522e37a73e260c73e3d",
      "d73088f9472d795523a2c0c75c2b48ee3576d361f4c68ff6b67a27007e943618",
      "5388df671b9ffe8e655109cb906e270dd021523154f5d55de79165e2c8f9e82e",
      "d92290ec46357c74f893e1c9a20b8b9ce541c3b9fc9aed13d33a19325f986ef3",
      "ff43ae70fc3a49fdadbeaa452210e9bce6a0c44f586fc946a73e4b4558414a24",
      "44f631b696b7c378d2228340c7af1ca3740a204426d5ecbf1cbd2d8d2ce63ad7",
      "0a692056bc332e90e11cfe87b8ac52bd4717b140ec443485d41b134d520d47b1",
      "fbf547b61c5e1e308fec0a93130e2bd526dd187c813dcb3b83f6ec542ca66e2e",
      "ba1889c74e2d4152bcc28dd7412906b3ffed4daff1dcb8346576a722827760ec",
      "e8fd2ee76c8cb29b84a9ae6f1459efea959d9b124b741aba3d69e0120061d356",
      "fd486efc9dde50a78424ee778bdd9dc545e843e464b79fc46fb4e89e4de6609e",
      "934b4a0d8c6f89de76bdf3656d78776ccc6d7cf9439a3ec2ca343189f0d9aff1",
      "5aefde81b5189f3cc7430aa1a0161d6973f5a81d39bb84ae8fbee3a2c47e148d",
      "54ecbcf4a1539976c614a49119f9c10cc2e5b0b8faeb8d7b52ec0e6046a6a109",
      "29f279edcff5b354b029ad444b5a89faa5e79495a876225b17eee8b839fd4cdf",
      "eec9bc799a45fa9173611bc9ff1a5effc733adc3d039cd06ccd0d0ec3b17d374",
      "c1796682424af56a8e514fd5eaba9af25ead34b83e6b242603b7c737f49c39f0",
      "55839f8e68c9b1b2c57456253523b94a18035219a3f642b599ed919d51e2b5d3",
      "c775a3e190232ca124580d1dc9422a457fbd46143d0dd51705a1592a5690c53d",
      "bbc21f0f44ecc0ca2905da09e909b2c7f3817aa3fb73cb8090ba61bf1894c6e0",
      "10705b887b830f8a407e9ca871fbf9e1c4346b54018d6013cce58769a9996607",
      "f89df58db5ca19ed56c3475210a6b41083ebef53fc8c2d473f7385fa61d2c14f",
      "fdce9f52de4bfc646f47cf442f44814d2e27e6e576966903faa353db30a1a062",
      "b0959a216be344b8b9eeeb6a75d53302c0738afd79d95c88f228461113404bc2",
      "db2f934f5aa2bfa46d2969a20e14fd0f00e2580de47729504c73c7f4bdcdc616",
      "c189f33802a1d9e83e51c98ec4d0158f853815055a59fa8bb52620fdea298ea1",
      "6847b3218beb972a32e5bfb0d07b199b28d04ed2e387759baf64e32df372492c",
      "64ff30960cde911868d6e7978d175712c7f0c909a87d290078c010d80c2eeb9c",
      "66e3750adca271c9709ae392f0abc6a4b02f8ca6a541a421096819adee6d714d",
      "357cd9b76877976bfee88e27c94e2e192dc01de762b1d178b36768573eb04ed8",
      "92118caec067ddec91e57fdd46b411c612a3bf63d1a70e8483488f180e40da3d",
      "a177be8db689c2a4797af3952e902179b6190b31a069c3cbda9c95cd32d485f2",
      "c07e79d4745dde4515d9051cd951c7ca2f83008ebf36f046a87e155e577641a4",
    };

    if (height() > 184200){
      for (const auto &kis : exchange_981_key_images)
      {
        crypto::key_image ki;
        epee::string_tools::hex_to_pod(kis, ki);
        if (!has_key_image(ki))
        {
          LOG_PRINT_L1("Fixup: adding missing spent key " << ki);
          add_spent_key(ki);
        }
      }
    }
  }
  batch_stop();
}

}  // namespace cryptonote
