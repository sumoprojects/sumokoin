// Copyright (c) 2020, The Monero Project
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

#include "levin_notify.h"

#include <boost/asio/steady_timer.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <deque>
#include <stdexcept>
#include <utility>

#include "common/expect.h"
#include "common/varint.h"
#include "cryptonote_config.h"
#include "crypto/crypto.h"
#include "crypto/duration.h"
#include "cryptonote_basic/connection_context.h"
#include "cryptonote_core/i_core_events.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "net/dandelionpp.h"
#include "p2p/net_node.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.p2p.tx"

namespace
{
  int get_command_from_message(const cryptonote::blobdata &msg)
  {
    return msg.size() >= sizeof(epee::levin::bucket_head2) ? SWAP32LE(((epee::levin::bucket_head2*)msg.data())->m_command) : 0;
  }
}

namespace cryptonote
{
namespace levin
{
  namespace
  {
    constexpr const std::size_t connection_id_reserve_size = 100;

    constexpr const std::chrono::minutes noise_min_epoch{CRYPTONOTE_NOISE_MIN_EPOCH};
    constexpr const std::chrono::seconds noise_epoch_range{CRYPTONOTE_NOISE_EPOCH_RANGE};

    constexpr const std::chrono::minutes dandelionpp_min_epoch{CRYPTONOTE_DANDELIONPP_MIN_EPOCH};
    constexpr const std::chrono::seconds dandelionpp_epoch_range{CRYPTONOTE_DANDELIONPP_EPOCH_RANGE};

    constexpr const std::chrono::seconds noise_min_delay{CRYPTONOTE_NOISE_MIN_DELAY};
    constexpr const std::chrono::seconds noise_delay_range{CRYPTONOTE_NOISE_DELAY_RANGE};

    /* A custom duration is used for the poisson distribution because of the
       variance. If 5 seconds is given to `std::poisson_distribution`, 95% of
       the values fall between 1-9s in 1s increments (not granular enough). If
       5000 milliseconds is given, 95% of the values fall between 4859ms-5141ms
       in 1ms increments (not enough time variance). Providing 20 quarter
       seconds yields 95% of the values between 3s-7.25s in 1/4s increments. */
    using fluff_stepsize = std::chrono::duration<std::chrono::milliseconds::rep, std::ratio<1, 4>>;
    constexpr const std::chrono::seconds fluff_average_in{CRYPTONOTE_DANDELIONPP_FLUSH_AVERAGE};

    /*! Bitcoin Core is using 1/2 average seconds for outgoing connections
        compared to incoming. The thinking is that the user controls outgoing
        connections (Dandelion++ makes similar assumptions in its stem
        algorithm). The randomization yields 95% values between 1s-4s in
	1/4s increments. */
  using fluff_duration = crypto::random_poisson_subseconds::result_type;
  constexpr const fluff_duration fluff_average_out{fluff_duration{fluff_average_in} / 2};

    /*! Select a randomized duration from 0 to `range`. The precision will be to
        the systems `steady_clock`. As an example, supplying 3 seconds to this
        function will select a duration from [0, 3] seconds, and the increments
        for the selection will be determined by the `steady_clock` precision
        (typically nanoseconds).

        \return A randomized duration from 0 to `range`. */
    std::chrono::steady_clock::duration random_duration(std::chrono::steady_clock::duration range)
    {
      using rep = std::chrono::steady_clock::rep;
      return std::chrono::steady_clock::duration{crypto::rand_range(rep(0), range.count())};
    }

    //! \return All outgoing connections supporting fragments in `connections`.
    std::vector<boost::uuids::uuid> get_out_connections(connections& p2p)
    {
      std::vector<boost::uuids::uuid> outs;
      outs.reserve(connection_id_reserve_size);

      /* The foreach call is serialized with a lock, but should be quick due to
         the reserve call so a strand is not used. Investigate if there is lots
         of waiting in here. */

      p2p.foreach_connection([&outs] (detail::p2p_context& context) {
        if (!context.m_is_income)
          outs.emplace_back(context.m_connection_id);
        return true;
      });

      return outs;
    }

    std::string make_tx_payload(std::vector<blobdata>&& txs, const bool pad, const bool fluff)
    {
      NOTIFY_NEW_TRANSACTIONS::request request{};
      request.txs = std::move(txs);
      request.dandelionpp_fluff = fluff;

      if (pad)
      {
        size_t bytes = 9 /* header */ + 4 /* 1 + 'txs' */ + tools::get_varint_data(request.txs.size()).size();
        for(auto tx_blob_it = request.txs.begin(); tx_blob_it!=request.txs.end(); ++tx_blob_it)
          bytes += tools::get_varint_data(tx_blob_it->size()).size() + tx_blob_it->size();

        // stuff some dummy bytes in to stay safe from traffic volume analysis
        static constexpr const size_t granularity = 1024;
        size_t padding = granularity - bytes % granularity;
        const size_t overhead = 2 /* 1 + '_' */ + tools::get_varint_data(padding).size();
        if (overhead > padding)
          padding = 0;
        else
          padding -= overhead;
        request._ = std::string(padding, ' ');

        std::string arg_buff;
        epee::serialization::store_t_to_binary(request, arg_buff);

        // we probably lowballed the payload size a bit, so added a but too much. Fix this now.
        size_t remove = arg_buff.size() % granularity;
        if (remove > request._.size())
          request._.clear();
        else
          request._.resize(request._.size() - remove);
        // if the size of _ moved enough, we might lose byte in size encoding, we don't care
      }

      std::string fullBlob;
      if (!epee::serialization::store_t_to_binary(request, fullBlob))
        throw std::runtime_error{"Failed to serialize to epee binary format"};

      return fullBlob;
    }

    bool make_payload_send_txs(connections& p2p, std::vector<blobdata>&& txs, const boost::uuids::uuid& destination, const bool pad, const bool fluff)
    {
      const cryptonote::blobdata blob = make_tx_payload(std::move(txs), pad, fluff);
      p2p.for_connection(destination, [&blob](detail::p2p_context& context) {
        on_levin_traffic(context, true, true, false, blob.size(), get_command_from_message(blob));
        return true;
      });
      return p2p.notify(NOTIFY_NEW_TRANSACTIONS::ID, epee::strspan<std::uint8_t>(blob), destination);
    }

    /* The current design uses `asio::strand`s. The documentation isn't as clear
       as it should be - a `strand` has an internal `mutex` and `bool`. The
       `mutex` synchronizes thread access and the `bool` is set when a thread is
       executing something "in the strand". Therefore, if a callback has lots of
       work to do in a `strand`, asio can switch to some other task instead of
       blocking 1+ threads to wait for the original thread to complete the task
       (as is the case when client code has a `mutex` inside the callback). The
       downside is that asio _always_ allocates for the callback, even if it can
       be immediately executed. So if all work in a strand is minimal, a lock
       may be better.

       This code uses a strand per "zone" and a strand per "channel in a zone".
       `dispatch` is used heavily, which means "execute immediately in _this_
       thread if the strand is not in use, otherwise queue the callback to be
       executed immediately after the strand completes its current task".
       `post` is used where deferred execution to an `asio::io_service::run`
       thread is preferred.

       The strand per "zone" is useful because the levin
       `foreach_connection` is blocked with a mutex anyway. So this primarily
       helps with reducing blocking of a thread attempting a "flood"
       notification. Updating/merging the outgoing connections in the
       Dandelion++ map is also somewhat expensive.

       The strand per "channel" may need a re-visit. The most "expensive" code
       is figuring out the noise/notification to send. If levin code is
       optimized further, it might be better to just use standard locks per
       channel. */

    //! A queue of levin messages for a noise i2p/tor link
    struct noise_channel
    {
      explicit noise_channel(boost::asio::io_service& io_service)
        : active(nullptr),
          queue(),
          strand(io_service),
          next_noise(io_service),
          connection(boost::uuids::nil_uuid())
      {}

      // `asio::io_service::strand` cannot be copied or moved
      noise_channel(const noise_channel&) = delete;
      noise_channel& operator=(const noise_channel&) = delete;

      // Only read/write these values "inside the strand"

      epee::byte_slice active;
      std::deque<epee::byte_slice> queue;
      boost::asio::io_service::strand strand;
      boost::asio::steady_timer next_noise;
      boost::uuids::uuid connection;
    };
  } // anonymous

  namespace detail
  {
    struct zone
    {
      explicit zone(boost::asio::io_service& io_service, std::shared_ptr<connections> p2p, epee::byte_slice noise_in, bool is_public, bool pad_txs)
        : p2p(std::move(p2p)),
          noise(std::move(noise_in)),
          next_epoch(io_service),
          flush_txs(io_service),
          strand(io_service),
          map(),
          channels(),
          flush_time(std::chrono::steady_clock::time_point::max()),
          connection_count(0),
          is_public(is_public),
          pad_txs(pad_txs),
          fluffing(false)
      {
        for (std::size_t count = 0; !noise.empty() && count < CRYPTONOTE_NOISE_CHANNELS; ++count)
          channels.emplace_back(io_service);
      }

      const std::shared_ptr<connections> p2p;
      const epee::byte_slice noise; //!< `!empty()` means zone is using noise channels
      boost::asio::steady_timer next_epoch;
      boost::asio::steady_timer flush_txs;
      boost::asio::io_service::strand strand;
      net::dandelionpp::connection_map map;//!< Tracks outgoing uuid's for noise channels or Dandelion++ stems
      std::deque<noise_channel> channels;  //!< Never touch after init; only update elements on `noise_channel.strand`
      std::chrono::steady_clock::time_point flush_time; //!< Next expected Dandelion++ fluff flush
      std::atomic<std::size_t> connection_count; //!< Only update in strand, can be read at any time
      const bool is_public;                      //!< Zone is public ipv4/ipv6 connections
      const bool pad_txs;                        //!< Pad txs to the next boundary for privacy
      bool fluffing;                             //!< Zone is in Dandelion++ fluff epoch
    };
  } // detail

  namespace
  {
    //! Adds a message to the sending queue of the channel.
    class queue_covert_notify
    {
      std::shared_ptr<detail::zone> zone_;
      epee::byte_slice message_; // Requires manual copy constructor
      const std::size_t destination_;

    public:
      queue_covert_notify(std::shared_ptr<detail::zone> zone, epee::byte_slice message, std::size_t destination)
        : zone_(std::move(zone)), message_(std::move(message)), destination_(destination)
      {}

      queue_covert_notify(queue_covert_notify&&) = default;
      queue_covert_notify(const queue_covert_notify& source)
        : zone_(source.zone_), message_(source.message_.clone()), destination_(source.destination_)
      {}

      //! \pre Called within `zone_->channels[destionation_].strand`.
      void operator()()
      {
        if (!zone_)
          return;

        noise_channel& channel = zone_->channels.at(destination_);
        assert(channel.strand.running_in_this_thread());

        if (!channel.connection.is_nil())
          channel.queue.push_back(std::move(message_));
        else if (destination_ == 0 && zone_->connection_count == 0)
          MWARNING("Unable to send transaction(s) over anonymity network - no available outbound connections");
      }
    };

    //! Sends txs on connections with expired timers, and queues callback for next timer expiration (if any).
    struct fluff_flush
    {
      std::shared_ptr<detail::zone> zone_;
      std::chrono::steady_clock::time_point flush_time_;

      static void queue(std::shared_ptr<detail::zone> zone, const std::chrono::steady_clock::time_point flush_time)
      {
        assert(zone != nullptr);
        assert(zone->strand.running_in_this_thread());

        detail::zone& this_zone = *zone;
        this_zone.flush_time = flush_time;
        this_zone.flush_txs.expires_at(flush_time);
        this_zone.flush_txs.async_wait(this_zone.strand.wrap(fluff_flush{std::move(zone), flush_time}));
      }

      void operator()(const boost::system::error_code error)
      {
        if (!zone_ || !zone_->p2p)
          return;

        assert(zone_->strand.running_in_this_thread());

        const bool timer_error = bool(error);
        if (timer_error)
        {
          if (error != boost::system::errc::operation_canceled)
            throw boost::system::system_error{error, "fluff_flush timer failed"};

          // new timer canceled this one set in future
          if (zone_->flush_time < flush_time_)
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        auto next_flush = std::chrono::steady_clock::time_point::max();
        std::vector<std::pair<std::vector<blobdata>, boost::uuids::uuid>> connections{};
        zone_->p2p->foreach_connection([timer_error, now, &next_flush, &connections] (detail::p2p_context& context)
        {
          if (!context.fluff_txs.empty())
          {
            if (context.flush_time <= now || timer_error) // flush on canceled timer
            {
              context.flush_time = std::chrono::steady_clock::time_point::max();
              connections.emplace_back(std::move(context.fluff_txs), context.m_connection_id);
              context.fluff_txs.clear();
            }
            else // not flushing yet
              next_flush = std::min(next_flush, context.flush_time);
          }
          else // nothing to flush
            context.flush_time = std::chrono::steady_clock::time_point::max();
          return true;
        });

        // Always send txs in stem mode over i2p/tor, see comments in `send_txs` below.
        for (auto& connection : connections)
        {
          std::sort(connection.first.begin(), connection.first.end()); // don't leak receive order
          make_payload_send_txs(*zone_->p2p, std::move(connection.first), connection.second, zone_->pad_txs, zone_->is_public);
        }

        if (next_flush != std::chrono::steady_clock::time_point::max())
          fluff_flush::queue(std::move(zone_), next_flush);
        else
          zone_->flush_time = next_flush; // signal that no timer is set
      }
    };

    /*! The "fluff" portion of the Dandelion++ algorithm. Every tx is queued
        per-connection and flushed with a randomized poisson timer. This
        implementation only has one system timer per-zone, and instead tracks
        the lowest flush time. */
    struct fluff_notify
    {
      std::shared_ptr<detail::zone> zone_;
      std::vector<blobdata> txs_;
      boost::uuids::uuid source_;

      void operator()()
      {
        run(std::move(zone_), epee::to_span(txs_), source_);
      }

      static void run(std::shared_ptr<detail::zone> zone, epee::span<const blobdata> txs, const boost::uuids::uuid& source)
      {
        if (!zone || !zone->p2p || txs.empty())
          return;

        assert(zone->strand.running_in_this_thread());

        const auto now = std::chrono::steady_clock::now();
        auto next_flush = std::chrono::steady_clock::time_point::max();

        crypto::random_poisson_subseconds in_duration(fluff_average_in);
        crypto::random_poisson_subseconds out_duration(fluff_average_out);


        MDEBUG("Queueing " << txs.size() << " transaction(s) for Dandelion++ fluffing");

        bool available = false;
        zone->p2p->foreach_connection([txs, now, &zone, &source, &in_duration, &out_duration, &next_flush, &available] (detail::p2p_context& context)
        {
          // When i2p/tor, only fluff to outbound connections
          if (source != context.m_connection_id && (zone->is_public || !context.m_is_income))
          {
            available = true;
            if (context.fluff_txs.empty())
              context.flush_time = now + (context.m_is_income ? in_duration() : out_duration());

            next_flush = std::min(next_flush, context.flush_time);
            context.fluff_txs.reserve(context.fluff_txs.size() + txs.size());
            for (const blobdata& tx : txs)
              context.fluff_txs.push_back(tx); // must copy instead of move (multiple conns)
          }
          return true;
        });

        if (!available)
          MWARNING("Unable to send transaction(s), no available connections");

       if (next_flush < zone->flush_time)
         fluff_flush::queue(std::move(zone), next_flush);
      }
    };

    //! Updates the connection for a channel.
    struct update_channel
    {
      std::shared_ptr<detail::zone> zone_;
      const std::size_t channel_;
      const boost::uuids::uuid connection_;

      //! \pre Called within `stem_.strand`.
      void operator()() const
      {
        if (!zone_)
          return;

        noise_channel& channel = zone_->channels.at(channel_);
        assert(channel.strand.running_in_this_thread());
        static_assert(
          CRYPTONOTE_MAX_FRAGMENTS <= (noise_min_epoch / (noise_min_delay + noise_delay_range)),
          "Max fragments more than the max that can be sent in an epoch"
        );

        /* This clears the active message so that a message "in-flight" is
           restarted. DO NOT try to send the remainder of the fragments, this
           additional send time can leak that this node was sending out a real
           notify (tx) instead of dummy noise. */

        channel.connection = connection_;
        channel.active = nullptr;

        if (connection_.is_nil())
          channel.queue.clear();
      }
    };

    //! Merges `out_connections_` into the existing `zone_->map`.
    struct update_channels
    {
      std::shared_ptr<detail::zone> zone_;
      std::vector<boost::uuids::uuid> out_connections_;

      //! \pre Called within `zone->strand`.
      static void post(std::shared_ptr<detail::zone> zone)
      {
        if (!zone)
          return;

        assert(zone->strand.running_in_this_thread());

        zone->connection_count = zone->map.size();

        // only noise uses the "noise channels", only update when enabled
        if (zone->noise.empty())
          return;

        for (auto id = zone->map.begin(); id != zone->map.end(); ++id)
        {
          const std::size_t i = id - zone->map.begin();
          zone->channels[i].strand.post(update_channel{zone, i, *id});
        }
      }

      //! \pre Called within `zone_->strand`.
      static void run(std::shared_ptr<detail::zone> zone, std::vector<boost::uuids::uuid> out_connections)
      {
        if (!zone)
          return;

        assert(zone->strand.running_in_this_thread());
        if (zone->map.update(std::move(out_connections)))
          post(std::move(zone));
      }

      //! \pre Called within `zone_->strand`.
      void operator()()
      {
        run(std::move(zone_), std::move(out_connections_));
      }
    };

    //! Checks fluff status for this node, and then does stem or fluff for txes
    struct dandelionpp_notify
    {
      std::shared_ptr<detail::zone> zone_;
      i_core_events* core_;
      std::vector<blobdata> txs_;
      boost::uuids::uuid source_;

      //! \pre Called in `zone_->strand`
      void operator()()
      {
        if (!zone_ || !core_ || txs_.empty())
          return;

        if (zone_->fluffing)
        {
          core_->on_transactions_relayed(epee::to_span(txs_), relay_method::fluff);
          fluff_notify::run(std::move(zone_), epee::to_span(txs_), source_);
        }
        else // forward tx in stem
        {
          core_->on_transactions_relayed(epee::to_span(txs_), relay_method::stem);
          for (int tries = 2; 0 < tries; tries--)
          {
            const boost::uuids::uuid destination = zone_->map.get_stem(source_);
            if (!destination.is_nil() && make_payload_send_txs(*zone_->p2p, std::vector<blobdata>{txs_}, destination, zone_->pad_txs, false))
            {
              /* Source is intentionally omitted in debug log for privacy - a
                  nil uuid indicates source is that node. */
              MDEBUG("Sent " << txs_.size() << " transaction(s) to " << destination << " using Dandelion++ stem");
              return;
            }

            // connection list may be outdated, try again
            update_channels::run(zone_, get_out_connections(*zone_->p2p));
          }

          MERROR("Unable to send transaction(s) via Dandelion++ stem");
        }
      }
    };

    //! Swaps out noise/dandelionpp channels entirely; new epoch start.
    class change_channels
    {
      std::shared_ptr<detail::zone> zone_;
      net::dandelionpp::connection_map map_; // Requires manual copy constructor
      bool fluffing_;

    public:
      explicit change_channels(std::shared_ptr<detail::zone> zone, net::dandelionpp::connection_map map, const bool fluffing)
        : zone_(std::move(zone)), map_(std::move(map)), fluffing_(fluffing)
      {}

      change_channels(change_channels&&) = default;
      change_channels(const change_channels& source)
        : zone_(source.zone_), map_(source.map_.clone())
      {}

      //! \pre Called within `zone_->strand`.
      void operator()()
      {
        if (!zone_)
          return;

        assert(zone_->strand.running_in_this_thread());

        if (zone_->is_public)
          MDEBUG("Starting new Dandelion++ epoch: " << (fluffing_ ? "fluff" : "stem"));

        zone_->map = std::move(map_);
        zone_->fluffing = fluffing_;
        update_channels::post(std::move(zone_));
      }
    };

    //! Sends a noise packet or real notification and sets timer for next call.
    struct send_noise
    {
      std::shared_ptr<detail::zone> zone_;
      const std::size_t channel_;

      static void wait(const std::chrono::steady_clock::time_point start, std::shared_ptr<detail::zone> zone, const std::size_t index)
      {
        if (!zone)
          return;

        noise_channel& channel = zone->channels.at(index);
        channel.next_noise.expires_at(start + noise_min_delay + random_duration(noise_delay_range));
        channel.next_noise.async_wait(
          channel.strand.wrap(send_noise{std::move(zone), index})
        );
      }

      //! \pre Called within `zone_->channels[channel_].strand`.
      void operator()(boost::system::error_code error)
      {
        if (!zone_ || !zone_->p2p || zone_->noise.empty())
          return;

        if (error && error != boost::system::errc::operation_canceled)
          throw boost::system::system_error{error, "send_noise timer failed"};

        assert(zone_->channels.at(channel_).strand.running_in_this_thread());

        const auto start = std::chrono::steady_clock::now();
        noise_channel& channel = zone_->channels.at(channel_);

        if (!channel.connection.is_nil())
        {
          epee::byte_slice message = nullptr;
          if (!channel.active.empty())
            message = channel.active.take_slice(zone_->noise.size());
          else if (!channel.queue.empty())
          {
            channel.active = channel.queue.front().clone();
            message = channel.active.take_slice(zone_->noise.size());
          }
          else
            message = zone_->noise.clone();

          zone_->p2p->for_connection(channel.connection, [&](detail::p2p_context& context) {
            on_levin_traffic(context, true, true, false, message.size(), "noise");
            return true;
          });
          if (zone_->p2p->send(std::move(message), channel.connection))
          {
            if (!channel.queue.empty() && channel.active.empty())
              channel.queue.pop_front();
          }
          else
          {
            channel.active = nullptr;
            channel.connection = boost::uuids::nil_uuid();

            auto connections = get_out_connections(*zone_->p2p);
            if (connections.empty())
              MWARNING("Lost all outbound connections to anonymity network - currently unable to send transaction(s)");

            zone_->strand.post(update_channels{zone_, std::move(connections)});
          }
        }

        wait(start, std::move(zone_), channel_);
      }
    };

    //! Prepares connections for new channel/dandelionpp epoch and sets timer for next epoch
    struct start_epoch
    {
      // Variables allow for Dandelion++ extension
      std::shared_ptr<detail::zone> zone_;
      std::chrono::seconds min_epoch_;
      std::chrono::seconds epoch_range_;
      std::size_t count_;

      //! \pre Should not be invoked within any strand to prevent blocking.
      void operator()(const boost::system::error_code error = {})
      {
        if (!zone_ || !zone_->p2p)
          return;

        if (error && error != boost::system::errc::operation_canceled)
          throw boost::system::system_error{error, "start_epoch timer failed"};

        const bool fluffing = crypto::rand_idx(unsigned(100)) < CRYPTONOTE_DANDELIONPP_FLUFF_PROBABILITY;
        const auto start = std::chrono::steady_clock::now();
        zone_->strand.dispatch(
          change_channels{zone_, net::dandelionpp::connection_map{get_out_connections(*(zone_->p2p)), count_}, fluffing}
        );

        detail::zone& alias = *zone_;
        alias.next_epoch.expires_at(start + min_epoch_ + random_duration(epoch_range_));
        alias.next_epoch.async_wait(start_epoch{std::move(*this)});
      }
    };
  } // anonymous

  notify::notify(boost::asio::io_service& service, std::shared_ptr<connections> p2p, epee::byte_slice noise, const bool is_public, const bool pad_txs)
    : zone_(std::make_shared<detail::zone>(service, std::move(p2p), std::move(noise), is_public, pad_txs))
  {
    if (!zone_->p2p)
      throw std::logic_error{"cryptonote::levin::notify cannot have nullptr p2p argument"};

    const bool noise_enabled = !zone_->noise.empty();
    if (noise_enabled || is_public)
    {
      const auto now = std::chrono::steady_clock::now();
      const auto min_epoch = noise_enabled ? noise_min_epoch : dandelionpp_min_epoch;
      const auto epoch_range = noise_enabled ? noise_epoch_range : dandelionpp_epoch_range;
      const std::size_t out_count = noise_enabled ? CRYPTONOTE_NOISE_CHANNELS : CRYPTONOTE_DANDELIONPP_STEMS;

      start_epoch{zone_, min_epoch, epoch_range, out_count}();

      for (std::size_t channel = 0; channel < zone_->channels.size(); ++channel)
        send_noise::wait(now, zone_, channel);
    }
  }

  notify::~notify() noexcept
  {}

  notify::status notify::get_status() const noexcept
  {
    if (!zone_)
      return {false, false};

    return {!zone_->noise.empty(), CRYPTONOTE_NOISE_CHANNELS <= zone_->connection_count};
  }

  void notify::new_out_connection()
  {
    if (!zone_ || zone_->noise.empty() || CRYPTONOTE_NOISE_CHANNELS <= zone_->connection_count)
      return;

    zone_->strand.dispatch(
      update_channels{zone_, get_out_connections(*(zone_->p2p))}
    );
  }

  void notify::run_epoch()
  {
    if (!zone_)
      return;
    zone_->next_epoch.cancel();
  }

  void notify::run_stems()
  {
    if (!zone_)
      return;

    for (noise_channel& channel : zone_->channels)
      channel.next_noise.cancel();
  }

  void notify::run_fluff()
  {
    if (!zone_)
      return;
    zone_->flush_txs.cancel();
  }

// suppress "this statement may fall through" warning on gcc (>7.0) and clang, dandelion cases below are supposed to fallthrough
#if __GNUC__ && defined( __has_warning )
#if __has_warning( "-Wimplicit-fallthrough=" )
#define SUPPRESS
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough="
#endif
#endif
  bool notify::send_txs(std::vector<blobdata> txs, const boost::uuids::uuid& source, i_core_events& core, relay_method tx_relay)
  {
    if (txs.empty())
      return true;

    if (!zone_)
      return false;

    /* If noise is enabled in a zone, it always takes precedence. The technique
        provides good protection against ISP adversaries, but not sybil
        adversaries. Noise is currently only enabled over I2P/Tor - those
        networks provide protection against sybil attacks (we only send to
        outgoing connections).
        If noise is disabled, Dandelion++ is used for public networks only.
        Dandelion++ over I2P/Tor should be an interesting case to investigate,
        but the mempool/stempool needs to know the zone a tx originated from to
        work properly. */

    if (!zone_->noise.empty() && !zone_->channels.empty())
    {
      // covert send in "noise" channel
      static_assert(
        CRYPTONOTE_MAX_FRAGMENTS * CRYPTONOTE_NOISE_BYTES <= LEVIN_DEFAULT_MAX_PACKET_SIZE, "most nodes will reject this fragment setting"
      );

      if (tx_relay == relay_method::stem)
      {
        MWARNING("Dandelion++ stem not supported over noise networks");
        tx_relay = relay_method::local; // do not put into stempool embargo (hopefully not there already!).
      }

      core.on_transactions_relayed(epee::to_span(txs), tx_relay);

      // Padding is not useful when using noise mode. Send as stem so receiver
      // forwards in Dandelion++ mode.
      const std::string payload = make_tx_payload(std::move(txs), false, false);
      epee::byte_slice message = epee::levin::make_fragmented_notify(
        zone_->noise, NOTIFY_NEW_TRANSACTIONS::ID, epee::strspan<std::uint8_t>(payload)
      );
      if (CRYPTONOTE_MAX_FRAGMENTS * zone_->noise.size() < message.size())
      {
        MERROR("notify::send_txs provided message exceeding covert fragment size");
        return false;
      }

      for (std::size_t channel = 0; channel < zone_->channels.size(); ++channel)
      {
        zone_->channels[channel].strand.dispatch(
          queue_covert_notify{zone_, message.clone(), channel}
        );
      }
    }
    else
    {
      switch (tx_relay)
      {
        default:
        case relay_method::none:
        case relay_method::block:
          return false;
        case relay_method::stem:
	      tx_relay = relay_method::fluff; // don't set stempool embargo when skipping to fluff
        /* fallthrough */
        case relay_method::local:
          if (zone_->is_public)
          {
	        // this will change a local tx to stem or fluff ...
            zone_->strand.dispatch(
              dandelionpp_notify{zone_, std::addressof(core), std::move(txs), source}
            );
            break;
          }
          /* fallthrough */
        case relay_method::fluff:
          core.on_transactions_relayed(epee::to_span(txs), tx_relay);
          zone_->strand.dispatch(fluff_notify{zone_, std::move(txs), source});
          break;
      }
    }
    return true;
  }
  #ifdef SUPPRESS
  #undef SUPPRESS
  #pragma GCC diagnostic pop
  #endif
} // levin
} // net
