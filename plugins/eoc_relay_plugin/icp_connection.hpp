#pragma once

#include <memory>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/plugin_interface.hpp>
#include <fc/network/message_buffer.hpp>
#include "icp_protocol.hpp"

namespace eosio{

    class eoc_relay_plugin;
}

namespace eoc_icp {

using namespace std;
using tcp = boost::asio::ip::tcp;
namespace ws = boost::beast::websocket;
using socket_ptr = std::shared_ptr<tcp::socket>;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::plugin_interface;

struct handshake_initializer {
      static void populate(icp_handshake_message &hello);
   };


class relay;
class icp_connection : public std::enable_shared_from_this<icp_connection> {
   public:
      explicit icp_connection( string endpoint );

      explicit icp_connection( socket_ptr s );
      ~icp_connection();
      void initialize();

      socket_ptr              socket;

      fc::message_buffer<1024*1024>    pending_message_buffer;
      fc::optional<std::size_t>        outstanding_read_bytes;
      vector<char>            blk_buffer;

      struct queued_write {
         std::shared_ptr<vector<char>> buff;
         std::function<void(boost::system::error_code, std::size_t)> callback;
      };
      deque<queued_write>     write_queue;
      deque<queued_write>     out_queue;
      fc::sha256              node_id;
      
      int16_t                 sent_handshake_count = 0;
      bool                    connecting = false;
      bool                    syncing = false;
      uint16_t                protocol_version  = 0;
      std::string                  peer_addr;
      std::unique_ptr<boost::asio::steady_timer> response_expected;
      relay*   my_impl;
      block_id_type          fork_head;
      uint32_t               fork_head_num = 0;
      
      icp_handshake_message       last_handshake_recv;
      icp_handshake_message       last_handshake_sent;
      head                  head_local;
      void update_local_head(const head& h) ;
      // Computed data
      double                         offset{0};       //!< peer offset

      static const size_t            ts_buffer_size{32};
      char                           ts[ts_buffer_size];          //!< working buffer for making human readable timestamps
      /** @} */
      icp_go_away_reason         no_retry = icp_no_reason;
      bool connected();
      bool current();
      void reset();
      void close();
        
      /** @} */

      
      const string peer_name();
    
      void stop_send();

      void enqueue( const icp_net_message &msg, bool trigger_send = true );
     
      void flush_queues();
      bool enqueue_sync_block();
      void request_sync_blocks (uint32_t start, uint32_t end);

      void cancel_wait();
      void sync_wait();
      void fetch_wait();
      void sync_timeout(boost::system::error_code ec);
      void fetch_timeout(boost::system::error_code ec);

      void queue_write(std::shared_ptr<vector<char>> buff,
                       bool trigger_send,
                       std::function<void(boost::system::error_code, std::size_t)> callback);
      void do_queue_write();
      bool process_next_message(relay& impl, uint32_t message_length);
      void send_handshake();
      void send_icp_net_msg(const icp_net_message& icpmsg);
       fc::optional<fc::variant_object> _logger_variant;
      const fc::variant_object& get_logger_variant()  {
         if (!_logger_variant) {
            boost::system::error_code ec;
            auto rep = socket->remote_endpoint(ec);
            string ip = ec ? "<unknown>" : rep.address().to_string();
            string port = ec ? "<unknown>" : std::to_string(rep.port());

            auto lep = socket->local_endpoint(ec);
            string lip = ec ? "<unknown>" : lep.address().to_string();
            string lport = ec ? "<unknown>" : std::to_string(lep.port());

            _logger_variant.emplace(fc::mutable_variant_object()
               ("_name", peer_name())
               ("_id", node_id)
               ("_sid", ((string)node_id).substr(0, 7))
               ("_ip", ip)
               ("_port", port)
               ("_lip", lip)
               ("_lport", lport)
            );
         }
         return *_logger_variant;
      }
   
     
   };


}
