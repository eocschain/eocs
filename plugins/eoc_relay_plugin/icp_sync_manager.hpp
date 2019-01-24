#pragma once

#include <memory>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/plugin_interface.hpp>

namespace eosio
{

class eoc_relay_plugin;
}

namespace eoc_icp
{

class icp_connection;
using namespace std;
using tcp = boost::asio::ip::tcp;
namespace ws = boost::beast::websocket;
using icp_connection_ptr = std::shared_ptr<icp_connection>;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::plugin_interface;

class dispatch_manager
{
 public:
   uint32_t just_send_it_max = 0;

   //vector<transaction_id_type> req_trx;

   //std::multimap<block_id_type, connection_ptr> received_blocks;
   //std::multimap<transaction_id_type, connection_ptr> received_transactions;

   //void bcast_transaction (const packed_transaction& msg);
   //void rejected_transaction (const transaction_id_type& msg);
   //void bcast_block (const signed_block& msg);
   //void rejected_block (const block_id_type &id);

   //void recv_block (connection_ptr conn, const block_id_type& msg, uint32_t bnum);
   //void recv_transaction(connection_ptr conn, const transaction_id_type& id);
   //void recv_notice (connection_ptr conn, const notice_message& msg, bool generated);

   void retry_fetch(icp_connection_ptr conn);
};

class icp_sync_manager
{
 private:
   enum stages
   {
      lib_catchup,
      head_catchup,
      in_sync
   };

   icp_connection_ptr source;
   stages state;

   chain_plugin *chain_plug = nullptr;

   constexpr auto stage_str(stages s);

 public:
   explicit icp_sync_manager(uint32_t span);
   void set_state(stages s);
   bool sync_required();
   void recv_handshake(icp_connection_ptr c, const icp_handshake_message &msg);
};

} // namespace eoc_icp
