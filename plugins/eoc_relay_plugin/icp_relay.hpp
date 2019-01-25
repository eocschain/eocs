#pragma once

#include <memory>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/plugin_interface.hpp>

#include "cache.hpp"
#include "api.hpp"
#include "icp_connection.hpp"
#include "icp_sync_manager.hpp"
namespace eosio{

    class eoc_relay_plugin;
    extern fc::logger icp_logger;
    extern  std::string  icp_peer_log_format;
}

namespace eoc_icp {

using namespace std;
using tcp = boost::asio::ip::tcp;
namespace ws = boost::beast::websocket;

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::plugin_interface;
using icp_connection_ptr = std::shared_ptr<icp_connection>;
using icp_connection_wptr = std::weak_ptr<icp_connection>;


class relay : public std::enable_shared_from_this<relay> {
public:
   friend class eosio::eoc_relay_plugin;
   friend struct handshake_initializer;
   friend class icp_connection;
   void start();
   void stop();

   read_only get_read_only_api() { return read_only{shared_from_this()}; }
   read_write get_read_write_api() { return read_write{shared_from_this()}; }

   void start_reconnect_timer();

   void update_local_head(bool force = false);

   void send(const icp_message& msg);
   void send_icp_net_msg(const icp_net_message& msg);
   void send_icp_notice_msg(const icp_notice_message& msg);

   void clear_cache_block_state();

   void open_channel(const block_header_state& seed);
   void push_transaction(vector<action> actions, function<void(bool)> callback = nullptr, packed_transaction::compression_type compression = packed_transaction::none);
   void handle_icp_actions(recv_transaction&& rt);
   chain::public_key_type get_authentication_key() const;
   chain::signature_type sign_compact(const chain::public_key_type& signer, const fc::sha256& digest) const;
   std::string p2p_address;
   std::string endpoint_address_;
   tcp::endpoint                    listen_endpoint;
   std::uint16_t endpoint_port_;
   std::uint32_t num_threads_ = 1;
   std::vector<std::string> connect_to_peers_;
   std::unique_ptr<tcp::acceptor>        acceptor;

   public_key_type id_ = fc::crypto::private_key::generate().get_public_key(); // random key to identify this process
   account_name local_contract_;
   account_name peer_contract_;
   chain_id_type peer_chain_id_;
   vector<chain::permission_level> signer_;
   flat_set<public_key_type> signer_required_keys_;

   head peer_head_;

   block_state_index block_states_;

    void handle_message( icp_connection_ptr c, const icp_handshake_message &msg);
    void handle_message( icp_connection_ptr c, const icp_go_away_message & msg);
    void handle_message( icp_connection_ptr c, const icp_notice_message & msg);
    void handle_message( icp_connection_ptr c, const channel_seed &msg );
   void handle_message( icp_connection_ptr c, const head_notice &msg );
   void handle_message( icp_connection_ptr c, const block_header_with_merkle_path &msg );
   void handle_message( icp_connection_ptr c, const icp_actions &msg );
   void handle_message( icp_connection_ptr c, const packet_receipt_request &msg );
   

    bool is_valid( const icp_handshake_message &msg);
    icp_connection_ptr find_connection( string host )const;
    void on_applied_transaction(const transaction_trace_ptr& t);
   void on_accepted_block(const block_state_with_action_digests_ptr& b);
   void on_irreversible_block(const block_state_ptr& s);
   void on_bad_block(const signed_block_ptr& b);

    void connect( icp_connection_ptr c );
      void connect( icp_connection_ptr c, tcp::resolver::iterator endpoint_itr );
      bool start_session( icp_connection_ptr c );
      void start_listen_loop( );
      void start_read_message( icp_connection_ptr c);

      void   close( icp_connection_ptr c );
      size_t count_open_sockets() const;

       std::set< icp_connection_ptr >       connections;

        void start_monitors( );
       void start_conn_timer(boost::asio::steady_timer::duration du, std::weak_ptr<icp_connection> from_connection);
       void connection_monitor(std::weak_ptr<icp_connection> from_connection);
       uint16_t to_protocol_version(uint16_t v);
        bool authenticate_peer(const icp_handshake_message& msg) const;
        void update_connections_head(const head& localhead);
      std::unique_ptr< dispatch_manager >   dispatcher;
        enum possible_connections : char {
         None = 0,
            Producers = 1 << 0,
            Specified = 1 << 1,
            Any = 1 << 2
            };
      possible_connections             allowed_connections{None};
      vector<string>                   supplied_peers;
      vector<chain::public_key_type>   allowed_peers; ///< peer keys allowed to connect
      std::map<chain::public_key_type,
               chain::private_key_type> private_keys; ///< overlapping with producer keys, also authenticating non-producing nodes

      chain_id_type                 chain_id;
      fc::sha256                    node_id; 
      std::unique_ptr< icp_sync_manager >       sync_master;
      bool                             done = false;
private:
   

   void cache_block_state(block_state_ptr b);

   void push_icp_actions(const sequence_ptr& s, recv_transaction&& rt);

   void cleanup();
   // void cleanup_sequences();
   void update_send_transaction_index(const send_transaction& t);
   std::unique_ptr<boost::asio::io_context> ioc_;
   std::vector<std::thread> socket_threads_;
   
   std::shared_ptr<boost::asio::deadline_timer> timer_; // only access on app io_service
 

   channels::applied_transaction::channel_type::handle on_applied_transaction_handle_;
   channels::accepted_block_with_action_digests::channel_type::handle on_accepted_block_handle_;
   channels::irreversible_block::channel_type::handle on_irreversible_block_handle_;
   channels::rejected_block::channel_type::handle on_bad_block_handle_;

   fc::microseconds tx_expiration_ = fc::seconds(300);
   uint8_t  tx_max_cpu_usage_ = 0;
   uint32_t tx_max_net_usage_ = 0;
   uint32_t delaysec_ = 0;

   fc::time_point last_transaction_time_ = fc::time_point::now();

   // uint32_t cumulative_cleanup_sequences_ = 0;
   uint32_t cumulative_cleanup_count_ = 0;

   send_transaction_index send_transactions_;
   block_with_action_digests_index block_with_action_digests_;
   recv_transaction_index recv_transactions_;
   uint32_t pending_schedule_version_ = 0;

   head local_head_;

  
   std::shared_ptr<tcp::resolver>     resolver;
    bool                          network_version_match = false;
     //std::unique_ptr< eoc_sync_manager >       sync_master;

      std::unique_ptr<boost::asio::steady_timer> connector_check;
      std::unique_ptr<boost::asio::steady_timer> transaction_check;
      std::unique_ptr<boost::asio::steady_timer> keepalive_timer;
      boost::asio::steady_timer::duration   connector_period;
      boost::asio::steady_timer::duration   txn_exp_period;
      boost::asio::steady_timer::duration   resp_expected_period;
      boost::asio::steady_timer::duration   keepalive_interval{std::chrono::seconds{32}};
      int                           max_cleanup_time_ms = 0;

      const std::chrono::system_clock::duration peer_authentication_interval{std::chrono::seconds{1}}; ///< Peer clock may be no more than 1 second skewed from our clock, including network latency.

      string                        user_agent_name;
      chain_plugin*                 chain_plug = nullptr;
      int                           started_sessions = 0;


       uint32_t                         max_client_count = 0;
      uint32_t                         max_nodes_per_host = 1;
      uint32_t                         num_clients = 0;

      //eosio::eoc_node_transaction_index        local_txns;

      

      bool                          use_socket_read_watermark = false;

      //eosio::channels::transaction_ack::channel_type::handle  incoming_transaction_ack_subscription;
};

struct icp_msgHandler : public fc::visitor<void> {
      relay &impl;
      icp_connection_ptr c;
      icp_msgHandler( relay &imp, icp_connection_ptr conn) : impl(imp), c(conn) {}

      template <typename T>
      void operator()(const T &msg) const
      {
         impl.handle_message( c, msg);
      }
   };

}
