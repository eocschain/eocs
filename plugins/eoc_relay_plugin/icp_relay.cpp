#include "icp_relay.hpp"

#include "api.hpp"
#include "message.hpp"
#include <fc/log/logger.hpp>

#include <eosio/eoc_relay_plugin/eoc_relay_plugin.hpp>
#include "icp_connection.hpp"
#include <eosio/producer_plugin/producer_plugin.hpp>

namespace eoc_icp {

 constexpr auto     icp_def_send_buffer_size_mb = 4;
constexpr auto     icp_def_send_buffer_size = 1024*1024*icp_def_send_buffer_size_mb;
constexpr auto     icp_message_header_size = 4;

  uint16_t icp_net_version_base = 0x04b5;
     uint16_t icp_net_version_range = 106;
 uint16_t icp_proto_explicit_sync = 1;
 uint16_t icp_net_version = icp_proto_explicit_sync;

 #define icp_peer_dlog( PEER, FORMAT, ... ) \
  FC_MULTILINE_MACRO_BEGIN \
   if( icp_logger.is_enabled( fc::log_level::debug ) ) \
      icp_logger.log( FC_LOG_MESSAGE( debug, icp_peer_log_format + FORMAT, __VA_ARGS__ (PEER->get_logger_variant()) ) ); \
  FC_MULTILINE_MACRO_END

#define icp_peer_ilog( PEER, FORMAT, ... ) \
  FC_MULTILINE_MACRO_BEGIN \
   if( icp_logger.is_enabled( fc::log_level::info ) ) \
      icp_logger.log( FC_LOG_MESSAGE( info, icp_peer_log_format + FORMAT, __VA_ARGS__ (PEER->get_logger_variant()) ) ); \
  FC_MULTILINE_MACRO_END

#define icp_peer_wlog( PEER, FORMAT, ... ) \
  FC_MULTILINE_MACRO_BEGIN \
   if( icp_logger.is_enabled( fc::log_level::warn ) ) \
      icp_logger.log( FC_LOG_MESSAGE( warn, icp_peer_log_format + FORMAT, __VA_ARGS__ (PEER->get_logger_variant()) ) ); \
  FC_MULTILINE_MACRO_END

#define icp_peer_elog( PEER, FORMAT, ... ) \
  FC_MULTILINE_MACRO_BEGIN \
   if( icp_logger.is_enabled( fc::log_level::error ) ) \
      icp_logger.log( FC_LOG_MESSAGE( error, icp_peer_log_format + FORMAT, __VA_ARGS__ (PEER->get_logger_variant())) ); \
  FC_MULTILINE_MACRO_END



void relay::start_monitors( )
{
    connector_check.reset(new boost::asio::steady_timer( app().get_io_service()));
      start_conn_timer(connector_period, std::weak_ptr<icp_connection>());
}

  void relay::start_conn_timer(boost::asio::steady_timer::duration du, std::weak_ptr<icp_connection> from_connection)
  {
      connector_check->expires_from_now( du);
      connector_check->async_wait( [this, from_connection](boost::system::error_code ec) {
            if( !ec) {
               connection_monitor(from_connection);
            }
            else {
               elog( "Error from connection check monitor: ${m}",( "m", ec.message()));
               start_conn_timer( connector_period, std::weak_ptr<icp_connection>());
            }
         });
  }

   void relay::connection_monitor(std::weak_ptr<icp_connection> from_connection)
   {
       auto max_time = fc::time_point::now();
      max_time += fc::milliseconds(max_cleanup_time_ms);
      auto from = from_connection.lock();
      auto it = (from ? connections.find(from) : connections.begin());
      if (it == connections.end()) it = connections.begin();
      while (it != connections.end()) {
         if (fc::time_point::now() >= max_time) {
            start_conn_timer(std::chrono::milliseconds(1), *it); // avoid exhausting
            return;
         }
         if( !(*it)->socket->is_open() && !(*it)->connecting) {
            if( (*it)->peer_addr.length() > 0) {
               connect(*it);
            }
            else {
               it = connections.erase(it);
               continue;
            }
         }
         ++it;
      }
      start_conn_timer(connector_period, std::weak_ptr<icp_connection>());
   }

void relay::start() {
   /*
   on_applied_transaction_handle_ = app().get_channel<channels::applied_transaction>().subscribe([this](transaction_trace_ptr t) {
      on_applied_transaction(t);
   });

   on_accepted_block_handle_ = app().get_channel<channels::accepted_block_with_action_digests>().subscribe([this](block_state_with_action_digests_ptr s) {
      on_accepted_block(s);
   });

   on_irreversible_block_handle_ = app().get_channel<channels::irreversible_block>().subscribe([this](block_state_ptr s) {
      on_irreversible_block(s);
   });

   on_bad_block_handle_ = app().get_channel<channels::rejected_block>().subscribe([this](signed_block_ptr b) {
      on_bad_block(b);
   });
   */
   ioc_ = std::make_unique<boost::asio::io_context>(num_threads_);

   timer_ = std::make_shared<boost::asio::deadline_timer>(app().get_io_service());

   auto address = boost::asio::ip::make_address(endpoint_address_);
   update_local_head();
    if( acceptor ) {
         acceptor->open(listen_endpoint.protocol());
         acceptor->set_option(tcp::acceptor::reuse_address(true));
         try {
           acceptor->bind(listen_endpoint);
         } catch (const std::exception& e) {
           ilog("eoc_relay_plugin::plugin_startup failed to bind to port ${port}",
             ("port", listen_endpoint.port()));
           throw e;
         }
         acceptor->listen();
         ilog("starting listener, max clients is ${mc}",("mc",max_client_count));
         start_listen_loop();
      }

}

void relay::stop() {
   
}



void relay::close(icp_connection_ptr c) {
      if( c->peer_addr.empty( ) && c->socket->is_open() ) {
         if (num_clients == 0) {
            fc_wlog( icp_logger, "num_clients already at 0");
         }
         else {
            --num_clients;
         }
      }
      c->close();
      
   }



 void relay::send_icp_notice_msg(const icp_notice_message& msg)
 {
    
    for( const auto& c : connections )
    {
       c->send_icp_net_msg(msg);
       ilog("send icp_notice_msg success");
    }
    
 }

 void relay::send_icp_net_msg(const icp_net_message& msg)
 {
    send_icp_notice_msg(icp_notice_message{local_head_});
    for( const auto& c : connections )
    {
       c->send_icp_net_msg(msg);
    }
       
 }

void relay::update_connections_head(const head& localhead)
{
   for( const auto& c : connections )
       c->update_local_head(localhead);
}

void relay::open_channel(const block_header_state& seed) {
   //send(channel_seed{seed});
   send_icp_net_msg(channel_seed{seed});
}

void relay::update_local_head(bool force) {
   // update local head
   head_ptr h;
   sequence_ptr s;
   try_catch([&h, &s, this]() mutable {
      h = get_read_only_api().get_head();
      s = get_read_only_api().get_sequence();
   });
   wlog("head: ${h}, ${hh}", ("h", bool(h))("hh", h ? *h : head{}));
   if (h and s and (force or h->head_block_id != local_head_.head_block_id
                     or h->last_irreversible_block_id != local_head_.last_irreversible_block_id)) { // local head changed

      local_head_ = *h;
      // wlog("head: ${h}", ("h", local_head_));

      std::unordered_set<packet_receipt_request> req_set; // deduplicate

      for (auto it = recv_transactions_.begin(); it != recv_transactions_.end();) {
         if (it->block_num > local_head_.last_irreversible_block_num) break;

         wlog("last_incoming_packet_seq: ${lp}, last_incoming_receipt_seq: ${lr}, start_packet_seq: ${sp}, start_receipt_seq: ${sr}", ("lp", s->last_incoming_packet_seq)("lr", s->last_incoming_receipt_seq)("sp", it->start_packet_seq)("sr", it->start_receipt_seq));
         auto req = s->make_genproof_request(it->start_packet_seq, it->start_receipt_seq);
         if (not req.empty()) {
            if (not req_set.count(req)) {
               //send(req);
               send_icp_net_msg(req);
               req_set.insert(req);
            }
            ++it;
            continue;
         }

         auto rt = *it;
         push_icp_actions(s, move(rt));
         it = recv_transactions_.erase(it);
      }

      update_connections_head(*h);
      send_icp_notice_msg(icp_notice_message{local_head_});
   }
}


 icp_connection_ptr relay::find_connection( string host )const
 {
    for( const auto& c : connections )
         if( c->peer_addr == host ) return c;
      return icp_connection_ptr();
 }

void relay::update_send_transaction_index(const send_transaction& t)
{
   auto &transaction_set = send_transactions_.get<by_id>();
   auto trace_iter = transaction_set.find(t.id);
   if(trace_iter == transaction_set.end())
   {
      ilog("insert transaction  ${id} success !!!",("id",t.id));
      send_transactions_.insert(t);
      return ;
   }

   bool b_change = send_transactions_.replace(trace_iter,t);
   if(!b_change)
   {
      ilog("update_send_transaction_index ${id} failed !!!",("id",t.id));
      return;
   }

   ilog("update transaction  ${id} success !!!",("id",t.id));
   
}


void relay::on_applied_transaction(const transaction_trace_ptr& t) {
   if(t->except){
      ilog("on_applied_transaction failed");
      return;
   }
   //ilog("on applied transaction");
   auto &transaction_set = send_transactions_.get<by_id>();
   auto trace_iter = transaction_set.find(t->id);
   if(trace_iter != transaction_set.end())
   {
      ilog("transaction_trace has found ${id}",("id",t->id));
   }
   send_transaction st{t->id, t->block_num};
   ilog("on applied transaction action dumy,${id},${b_num}",("id",t->id)("b_num",t->block_num));

   for (auto& action: t->action_traces) {
      if (action.receipt.receiver != action.act.account) 
      {
         ilog("action.receipt.receiver != action.act.account");
         continue;
      }
      if (action.act.account != local_contract_ or action.act.name == ACTION_DUMMY) { // thirdparty contract call or icp contract dummy call
         //ilog("on applied transaction action dumy!!!");
         for (auto& in: action.inline_traces) {
            if (in.receipt.receiver != in.act.account) continue;
            if (in.act.account == local_contract_ and in.act.name == ACTION_SENDACTION) {
               for (auto &inin: in.inline_traces) {
                  if (inin.receipt.receiver != inin.act.account) continue;
                  if (inin.act.name == ACTION_ISPACKET) {
                     auto seq = icp_packet::get_seq(inin.act.data, st.start_packet_seq);
                     st.packet_actions[seq] = send_transaction_internal{ACTION_ONPACKET, inin.act, inin.receipt};
                  }
               }
            }
         }
      } else if (action.act.account == local_contract_) {
         if (action.act.name == ACTION_ADDBLOCKS or action.act.name == ACTION_OPENCHANNEL) {
            ilog( "on_applied_transaction ,action name is ${p}",("p", action.act.name) );
            app().get_io_service().post([this] {
               update_local_head();
            });
         } else if (action.act.name == ACTION_ONPACKET) {
            for (auto &in: action.inline_traces) {
               if (in.receipt.receiver != in.act.account) continue;
               if (in.act.name == ACTION_ISRECEIPT) {
                  auto seq = icp_receipt::get_seq(in.act.data, st.start_receipt_seq);
                  st.receipt_actions[seq] = send_transaction_internal{ACTION_ONRECEIPT, in.act, in.receipt};
               }
            }
         } else if (action.act.name == ACTION_GENPROOF) {
            ilog("on applied transaction action ACTION_GENPROOF!!!");
            for (auto &in: action.inline_traces) {
               if (in.receipt.receiver != in.act.account) continue;
               if (in.act.name == ACTION_ISPACKET) {
                  auto seq = icp_packet::get_seq(in.act.data, st.start_packet_seq);
                  st.packet_actions[seq] = send_transaction_internal{ACTION_ONPACKET, in.act, in.receipt};
               } else if (in.act.name == ACTION_ISRECEIPT) {
                  auto seq = icp_receipt::get_seq(in.act.data, st.start_receipt_seq);
                  st.receipt_actions[seq] = send_transaction_internal{ACTION_ONRECEIPT, in.act, in.receipt};
               } else if (in.act.name == ACTION_ISRECEIPTEND) {
                  st.receiptend_actions.push_back(send_transaction_internal{ACTION_ONRECEIPTEND, in.act, in.receipt});
               }
            }
         /*} else if (action.act.name == ACTION_CLEANUP) {
            for (auto &in: action.inline_traces) {
               if (in.receipt.receiver != in.act.account) continue;
               if (in.act.name == ACTION_ISCLEANUP) {
                  st.cleanup_actions.push_back(send_transaction_internal{ACTION_ONCLEANUP, in.act, in.receipt});
               }
            }*/
         } else if (action.act.name == ACTION_ONRECEIPT) {
            ilog("on applied transaction action ACTION_ONRECEIPT!!!");
            for (auto &in: action.inline_traces) {
               if (in.receipt.receiver != in.act.account) continue;
               if (in.act.name == ACTION_ISRECEIPTEND) {
                  st.receiptend_actions.push_back(send_transaction_internal{ACTION_ONRECEIPTEND, in.act, in.receipt});
               }
            }
            // cleanup_sequences();
         }
      }
   }

   if (st.empty()) return;

   //send_transactions_.insert(st);
    update_send_transaction_index(st);
}

void relay::clear_cache_block_state() {
   block_states_.clear();
   wlog("clear_cache_block_state");
}

 bool relay::is_valid( const icp_handshake_message &msg)
 {
     bool valid = true;
      if (msg.last_irreversible_block_num > msg.head_num) {
         wlog("Handshake message validation: last irreversible block (${i}) is greater than head block (${h})",
              ("i", msg.last_irreversible_block_num)("h", msg.head_num));
         valid = false;
      }
      if (msg.p2p_address.empty()) {
         wlog("Handshake message validation: p2p_address is null string");
         valid = false;
      }
      if (msg.os.empty()) {
         wlog("Handshake message validation: os field is null string");
         valid = false;
      }
      if ((msg.sig != chain::signature_type() || msg.token != sha256()) && (msg.token != fc::sha256::hash(msg.time))) {
         wlog("Handshake message validation: token field invalid");
         valid = false;
      }
      return valid;

 }

 void relay::handle_message( icp_connection_ptr c, const icp_notice_message & msg)
 {
     ilog("received icp_notice_message");
      if (not msg.local_head_.valid()) return;

   app().get_io_service().post([=, self=shared_from_this()] {
      if (not peer_head_.valid()) {
         clear_cache_block_state();
      }
      peer_head_ = msg.local_head_;
      });

    // TODO: check validity
    icp_peer_ilog(c, "received icp_notice_message");
    ilog("received icp_notice_message");
   
 }

 void relay::handle_message( icp_connection_ptr c, const icp_go_away_message & msg)
 {
      string rsn = reason_str( msg.reason );
      icp_peer_ilog(c, "received go_away_message");
       ilog("received icp_go_away_message");
      ilog( "received a go away message from ${p}, reason = ${r}",
            ("p", c->peer_name())("r",rsn));
      c->no_retry = msg.reason;
      if(msg.reason == icp_duplicate ) {
         c->node_id = msg.node_id;
      }
      c->flush_queues();
      close (c);
 }

 void relay::handle_message( icp_connection_ptr c, const icp_handshake_message &msg)
 {
     icp_peer_ilog(c, "received handshake_message"); 
     ilog("received handshake_message");
      if (!is_valid(msg)) {
         icp_peer_elog( c, "bad handshake message");
         c->enqueue( icp_go_away_message( icp_fatal_other ));
         return;
      }
      controller& cc = chain_plug->chain();
      
      if( c->connecting ) {
         c->connecting = false;
      }
      
      if (msg.generation == 1) {
         if( msg.node_id == node_id) {
            elog( "Self connection detected. Closing connection");
            c->enqueue( icp_go_away_message( icp_self ) );
            return;
         }

         //judge peer chain id
         if( peer_chain_id_ != msg.chain_id)
         {
            elog( "different chain id");
            elog("peer_chain_id_ is ${ep}",
                    ("ep", peer_chain_id_));
            elog("msg.chain_id is ${ep}",
                    ("ep", msg.chain_id));
            c->enqueue( icp_go_away_message( icp_different_chainid ) );
            return;
         }
         
         if( c->peer_addr.empty() || c->last_handshake_recv.node_id == fc::sha256()) {
            fc_dlog(icp_logger, "checking for duplicate" );
            for(const auto &check : connections) {
               if(check == c)
                  continue;
               if(check->connected() && check->peer_name() == msg.p2p_address) {
                  elog("check->connected() is ${ep}",("ep",check->connected() ));
                  elog("check->peer_name() is ${ep}",("ep",check->peer_name() ));
                  // It's possible that both peers could arrive here at relatively the same time, so
                  // we need to avoid the case where they would both tell a different connection to go away.
                  // Using the sum of the initial handshake times of the two connections, we will
                  // arbitrarily (but consistently between the two peers) keep one of them.
                  if (msg.time + c->last_handshake_sent.time <= check->last_handshake_sent.time + check->last_handshake_recv.time)
                     continue;
                  elog("sending go_away duplicate to ${ep}",
                    ("ep", msg.p2p_address));
                  fc_dlog( icp_logger, "sending go_away duplicate to ${ep}", ("ep",msg.p2p_address) );
                  icp_go_away_message gam(icp_duplicate);
                  gam.node_id = node_id;
                  c->enqueue(gam);
                  c->no_retry = icp_duplicate;
                  return;
               }
            }
         }     
         else {
            fc_dlog(icp_logger, "skipping duplicate check, addr == ${pa}, id = ${ni}",("pa",c->peer_addr)("ni",c->last_handshake_recv.node_id));
         }

         c->protocol_version = to_protocol_version(msg.network_version);
         if(c->protocol_version != icp_net_version) {
            if (network_version_match) {
               elog("Peer network version does not match expected ${nv} but got ${mnv}",
                    ("nv", icp_net_version)("mnv", c->protocol_version));
               c->enqueue(icp_go_away_message(icp_wrong_version));
               return;
            } else {
               ilog("Local network version: ${nv} Remote version: ${mnv}",
                    ("nv", icp_net_version)("mnv", c->protocol_version));
            }
         }

         if(  c->node_id != msg.node_id) {
            c->node_id = msg.node_id;
         }

         if(!authenticate_peer(msg)) {
            elog("Peer not authenticated.  Closing connection.");
            c->enqueue(icp_go_away_message(icp_authentication));
            return;
         }
         if (c->sent_handshake_count == 0) {
           //send_icp_notice_msg(icp_notice_message{local_head_});
           elog("c->sent_handshake_count is ${nv} ",("nv",c->sent_handshake_count));
           elog("send->handshake message");
           c->send_handshake();
        }
      }
      
      c->last_handshake_recv = msg;
      c->_logger_variant.reset();
      //ilog("begin send local_head...................................");
      if(local_head_.valid())
      {
         c->enqueue(icp_notice_message{local_head_});
       //  ilog("send local_head.......................................");
      }
      
      sync_master->recv_handshake(c,msg);
      
 }

void relay::handle_message( icp_connection_ptr c, const channel_seed &s )
{
    ilog("received channel_seed");
   auto data = fc::raw::pack(bytes_data{fc::raw::pack(s.seed)});
   app().get_io_service().post([=, self=shared_from_this()] {
      action a;
      a.name = ACTION_OPENCHANNEL;
      a.data = data;
      ilog("data: ${d}", ("d", a.data));
      a.authorization.push_back(permission_level{local_contract_, permission_name("active")});
      push_transaction(vector<action>{a});
   });
}
   void relay::handle_message( icp_connection_ptr c, const head_notice &h )
   {
      ilog("received head_notice");
      if (not h.head_instance.valid()) return;

   app().get_io_service().post([=, self=shared_from_this()] {
      if (not peer_head_.valid()) {
         clear_cache_block_state();
      }
      peer_head_ = h.head_instance; // TODO: check validity
   });

   }
   void relay::handle_message( icp_connection_ptr c, const block_header_with_merkle_path &b )
   {
       auto ro = get_read_only_api();
   auto head = ro.get_head();

   if (not head) {
      elog("local head not found, maybe icp channel not opened");
      return;
   }

   auto first_num = b.block_header.block_num;
   if (not b.merkle_path.empty()) {
      first_num = block_header::num_from_id(b.merkle_path.front());
   }

    wlog("block_header_with_merkle_path: ${n1} -> ${n2}, head: ${h}", ("n1", first_num)("n2", b.block_header.block_num)("h", head->head_block_num));

   if (first_num != head->head_block_num) {
      // elog("unlinkable block: has ${has}, got ${got}", ("has", head->head_block_num)("got", first_num));
      return;
   }
   // TODO: more check and workaround

   auto data = fc::raw::pack(bytes_data{fc::raw::pack(b)});

   app().get_io_service().post([=, self=shared_from_this()] {
      action a;
      a.name = ACTION_ADDBLOCKS;
      a.data = data;
      push_transaction(vector<action>{a}, [this, self](bool success) {
         if (success) update_local_head();
      });
   });
   }
   void relay::handle_message( icp_connection_ptr c, const icp_actions &ia )
   {
      //ilog("received icp_actions");
      auto block_id = ia.block_header_instance.id();
   auto block_num = ia.block_header_instance.block_num();
   recv_transaction rt{block_num, block_id, ia.start_packet_seq, ia.start_receipt_seq};
   ilog("received icp_actions ${b_num},${p_seq},${r_seq}",("b_num",block_num)("p_seq",ia.start_packet_seq)("r_seq",ia.start_receipt_seq));
   auto ro = get_read_only_api();
   auto r = ro.get_block(read_only::get_block_params{block_id});
   if (r.block.is_null() or r.block["action_mroot"].as<fc::sha256>() == fc::sha256()) { // not exist or has no `action_mroot`
      auto data = fc::raw::pack(bytes_data{fc::raw::pack(ia.block_header_instance)});
      action a;
      a.name = ACTION_ADDBLOCK;
      a.data = data;
      rt.action_add_block = a;
   }

   for (auto& p: ia.packet_actions) {
      auto& s = p.second;
      action a;
      a.name = s.peer_action;
      a.data = fc::raw::pack(icp_action{fc::raw::pack(s.action_instance), fc::raw::pack(s.action_receipt_instance), block_id, fc::raw::pack(ia.action_digests)});
      rt.packet_actions.emplace_back(p.first, a);
   }
   for (auto& r: ia.receipt_actions) {
      auto& s = r.second;
      action a;
      a.name = s.peer_action;
      a.data = fc::raw::pack(icp_action{fc::raw::pack(s.action_instance), fc::raw::pack(s.action_receipt_instance), block_id, fc::raw::pack(ia.action_digests)});
      rt.receipt_actions.emplace_back(r.first, a);
   }
   for (auto& c: ia.receiptend_actions) {
      action a;
      a.name = c.peer_action;
      a.data = fc::raw::pack(icp_action{fc::raw::pack(c.action_instance), fc::raw::pack(c.action_receipt_instance), block_id, fc::raw::pack(ia.action_digests)});
      rt.receiptend_actions.emplace_back(a);
   }

   app().get_io_service().post([=, self=shared_from_this()]() mutable {
      handle_icp_actions(move(rt));
   });
   }
   void relay::handle_message( icp_connection_ptr c, const packet_receipt_request &req )
   {
       ilog("received packet_receipt_request");
       app().get_io_service().post([=, self=shared_from_this()] {
      action a;
      a.name = ACTION_GENPROOF;
      a.data = fc::raw::pack(req);
      push_transaction(vector<action>{a});
   });

   }



uint16_t relay::to_protocol_version(uint16_t v)
{
    if (v >= icp_net_version_base) {
         v -= icp_net_version_base;
         return (v > icp_net_version_range) ? 0 : v;
      }
      return 0;
}

bool relay::authenticate_peer(const icp_handshake_message& msg) const
{
     if(allowed_connections == None)
         return false;

      if(allowed_connections == Any)
         return true;

      if(allowed_connections & (Producers | Specified)) {
         auto allowed_it = std::find(allowed_peers.begin(), allowed_peers.end(), msg.key);
         auto private_it = private_keys.find(msg.key);
         bool found_producer_key = false;
         producer_plugin* pp = app().find_plugin<producer_plugin>();
         if(pp != nullptr)
            found_producer_key = pp->is_producer_key(msg.key);
         if( allowed_it == allowed_peers.end() && private_it == private_keys.end() && !found_producer_key) {
            elog( "Peer ${peer} sent a handshake with an unauthorized key: ${key}.",
                  ("peer", msg.p2p_address)("key", msg.key));
            return false;
         }
      }

      namespace sc = std::chrono;
      sc::system_clock::duration msg_time(msg.time);
      auto time = sc::system_clock::now().time_since_epoch();
      if(time - msg_time > peer_authentication_interval) {
         elog( "Peer ${peer} sent a handshake with a timestamp skewed by more than ${time}.",
               ("peer", msg.p2p_address)("time", "1 second")); // TODO Add to_variant for std::chrono::system_clock::duration
         return false;
      }
      
      if(msg.sig != chain::signature_type() && msg.token != sha256()) {
         sha256 hash = fc::sha256::hash(msg.time);
         if(hash != msg.token) {
            elog( "Peer ${peer} sent a handshake with an invalid token.",
                  ("peer", msg.p2p_address));
            return false;
         }
         chain::public_key_type peer_key;
         try {
            peer_key = crypto::public_key(msg.sig, msg.token, true);
         }
         catch (fc::exception& e) {
            elog( "Peer ${peer} sent a handshake with an unrecoverable key.",
                  ("peer", msg.p2p_address));
            return false;
         }
         if((allowed_connections & (Producers | Specified)) && peer_key != msg.key) {
            elog( "Peer ${peer} sent a handshake with an unauthenticated key.",
                  ("peer", msg.p2p_address));
            return false;
         }
      }
      else if(allowed_connections & (Producers | Specified)) {
         dlog( "Peer sent a handshake with blank signature and token, but this node accepts only authenticated connections.");
         return false;
      }
      
      return true;
}


void relay::cache_block_state(block_state_ptr b) {
   auto& idx = block_states_.get<by_num>();
   for (auto it = idx.begin(); it != idx.end();) {
      if (it->block_num + 500 < b->block_num) {
         it = idx.erase(it);
      } else {
         break;
      }
   }
   auto it = idx.find(b->block_num);
   if (it != idx.end()) {
      idx.erase(it);
   }

   block_states_.insert(static_cast<const block_header_state&>(*b));
   // wlog("cache_block_state");
}

void relay::on_accepted_block(const block_state_with_action_digests_ptr& b) {
    //ilog("on_accepted_block");
   bool must_send = false;
   bool may_send = false;

   auto& s = b->block_state;

   if (not peer_head_.valid()) {
      cache_block_state(s);
   }

   // new pending schedule
   if (s->header.new_producers.valid()) {
      must_send = true;
      pending_schedule_version_ = s->pending_schedule.version;
   }

   // new active schedule
   if (s->active_schedule.version > 0 and s->active_schedule.version == pending_schedule_version_) {
      must_send = true;
      pending_schedule_version_ = 0; // reset
   }

   for (auto& t: s->block->transactions) {
      auto id = t.trx.contains<transaction_id_type>() ? t.trx.get<transaction_id_type>() : t.trx.get<packed_transaction>().id();
      auto it = send_transactions_.find(id);
      if (it != send_transactions_.end()) {
         may_send = true;

         if (block_with_action_digests_.find(s->id) == block_with_action_digests_.end()) {
            block_with_action_digests_.insert(block_with_action_digests{s->id, b->action_digests});
         }

         break;
      }
   }

   ilog ("s->block_num, ${s->block_num}, peer_head_.head_block_num, ${peer_head_.head_block_num},${acdig}",
   ("s->block_num",s->block_num)("peer_head_.head_block_num",peer_head_.head_block_num)("acdig",b->action_digests));
   if (not must_send and peer_head_.valid() and s->block_num >= peer_head_.head_block_num) {
      ilog("enter must send judge!!!");
      auto lag = s->block_num - peer_head_.head_block_num;
      if ((may_send and lag >= MIN_CACHED_BLOCKS) or lag >= MAX_CACHED_BLOCKS) {
         must_send = true;
      }
   }

   if (must_send) {
      ilog ("must_send, ${must_send}",("must_send",must_send));
      auto& chain = app().get_plugin<chain_plugin>();
      vector<block_id_type> merkle_path;
      for (uint32_t i = peer_head_.head_block_num; i < s->block_num; ++i) {
         merkle_path.push_back(chain.chain().get_block_id_for_num(i));
      }

      //send(block_header_with_merkle_path{*s, merkle_path});
      send_icp_net_msg(block_header_with_merkle_path{*s, merkle_path});
   } else {
      send_icp_notice_msg(icp_notice_message{local_head_});
   }

   try_catch([this]() mutable {
      cleanup();
   });
}

void relay::on_irreversible_block(const block_state_ptr& s) {
    //ilog("on_irreversible_block");
   vector<send_transaction> txs;
   for (auto& t: s->block->transactions) {
      auto id = t.trx.contains<transaction_id_type>() ? t.trx.get<transaction_id_type>() : t.trx.get<packed_transaction>().id();
      auto it = send_transactions_.find(id);
      if (it != send_transactions_.end()) txs.push_back(*it);
   }


  // if (txs.empty()) {
      if (fc::time_point::now() - last_transaction_time_ >= fc::seconds(DUMMY_ICP_SECONDS) and peer_head_.valid()) {
         app().get_io_service().post([=] {
            action a;
            a.name = ACTION_DUMMY;
            a.data = fc::raw::pack(dummy{signer_[0].actor});
            push_transaction(vector<action>{a});
         });
         last_transaction_time_ = fc::time_point::now();
      }
   //   return;
   //}

   //last_transaction_time_ = fc::time_point::now();

   auto bit = block_with_action_digests_.find(s->id);
   if (bit == block_with_action_digests_.end()) {
      elog("cannot find block action digests: block id ${id}", ("id", s->id));
      return;
   }
    ilog ("s->block_num, ${s->block_num}, peer_head_.head_block_num, ${peer_head_.head_block_num}",
   ("s->block_num",s->block_num)("peer_head_.head_block_num",peer_head_.head_block_num));
   if (s->block_num > peer_head_.head_block_num) {
      auto& chain = app().get_plugin<chain_plugin>();
      vector<block_id_type> merkle_path;
      for (uint32_t i = peer_head_.head_block_num; i < s->block_num; ++i) {
         merkle_path.push_back(chain.chain().get_block_id_for_num(i));
      }

     // send(block_header_with_merkle_path{*s, merkle_path});
     ilog("send merkle message");
      send_icp_net_msg(block_header_with_merkle_path{*s, merkle_path});
   }

   icp_actions ia;
   ia.block_header_instance = static_cast<block_header>(s->header);
   ia.action_digests = bit->action_digests;

   std::map<uint64_t, send_transaction_internal> packet_actions; // key is packet seq
   std::map<uint64_t, send_transaction_internal> receipt_actions; // key is receipt seq
   for (auto& t: txs) {
      packet_actions.insert(t.packet_actions.cbegin(), t.packet_actions.cend());
      receipt_actions.insert(t.receipt_actions.cbegin(), t.receipt_actions.cend());
      for (auto& c: t.receiptend_actions) {
         ia.receiptend_actions.push_back(c);
      }
      ia.set_seq(t.start_packet_seq, t.start_receipt_seq);
   }
   for (auto& p: packet_actions) {
      ia.packet_actions.emplace_back(p.first, p.second);
   }
   for (auto& r: receipt_actions) {
      ia.receipt_actions.emplace_back(r.first, r.second);
   }

   //send(ia);
   ilog("send icp_actions ${b_num},${p_seq},${r_seq}",("b_num",s->block_num)("p_seq",ia.start_packet_seq)("r_seq",ia.action_digests));
   send_icp_net_msg(ia);
}

void relay::on_bad_block(const signed_block_ptr& b) {
}

chain::public_key_type relay::get_authentication_key() const
{
    if(!private_keys.empty())
         return private_keys.begin()->first;
      /*producer_plugin* pp = app().find_plugin<producer_plugin>();
      if(pp != nullptr && pp->get_state() == abstract_plugin::started)
         return pp->first_producer_public_key();*/
      return chain::public_key_type();
}

chain::signature_type relay::sign_compact(const chain::public_key_type& signer, const fc::sha256& digest) const
{
    auto private_key_itr = private_keys.find(signer);
      if(private_key_itr != private_keys.end())
         return private_key_itr->second.sign(digest);
      producer_plugin* pp = app().find_plugin<producer_plugin>();
      if(pp != nullptr && pp->get_state() == abstract_plugin::started)
         return pp->sign_compact(signer, digest);
      return chain::signature_type();
}


void relay::handle_icp_actions(recv_transaction&& rt) {
   if (local_head_.last_irreversible_block_num < rt.block_num) {
      recv_transactions_.insert(move(rt)); // cache it, push later
      return;
   }

   auto s = get_read_only_api().get_sequence();
   if (s) {
      auto req = s->make_genproof_request(rt.start_packet_seq, rt.start_receipt_seq);
      if (not req.empty()) {
         //send(req);
         send_icp_net_msg(req);
         recv_transactions_.insert(move(rt)); // cache it, push later
         return;
      }
   } else {
      elog("got empty sequence");
   }

   push_icp_actions(s, move(rt));
}

void relay::push_icp_actions(const sequence_ptr& s, recv_transaction&& rt) {
   app().get_io_service().post([=] {
      if (not rt.action_add_block.name.empty()) {
         push_transaction(vector<action>{rt.action_add_block});
      }

      // TODO: rate limiting, cache, and retry
      auto packet_seq = s->last_incoming_packet_seq + 1; // strictly increasing, otherwise fail
      auto receipt_seq = s->last_incoming_receipt_seq + 1; // strictly increasing, otherwise fail
      for (auto& p: rt.packet_actions) {
         if (p.first != packet_seq) continue;
         push_transaction(vector<action>{p.second});
         ++packet_seq;
      }
      for (auto& r: rt.receipt_actions) {
         wlog("last_incoming_receipt_seq: ${lr}, receipt seq: ${rs}", ("lr", receipt_seq)("rs", r.first));
         if (r.first != receipt_seq) continue;
         push_transaction(vector<action>{r.second});
         ++receipt_seq;
      }
      for (auto& a: rt.receiptend_actions) {
         push_transaction(vector<action>{a});
      }
   });
}

void relay::cleanup() {
   if (not local_head_.valid()) return;

   ++cumulative_cleanup_count_;
   if (cumulative_cleanup_count_ < MAX_CLEANUP_NUM) return;

   auto s = get_read_only_api().get_sequence(true);
   if (not s) {
      elog("got empty sequence");
      return;
   }

   wlog("sequence: ${s}", ("s", *s));

   auto cleanup_packet = (s->min_packet_seq > 0 and s->last_incoming_receipt_seq > s->min_packet_seq) ? (s->last_incoming_receipt_seq - s->min_packet_seq) : 0; // TODO: consistent receipt and packet sequence?
   auto cleanup_receipt = (s->min_receipt_seq > 0 and s->last_finalised_outgoing_receipt_seq > s->min_receipt_seq) ? (s->last_finalised_outgoing_receipt_seq - s->min_receipt_seq) : 0;
   auto cleanup_block = (s->min_block_num > 0 and s->max_finished_block_num() > s->min_block_num) ? (s->max_finished_block_num() - s->min_block_num) : 0;
   if (cleanup_packet >= MAX_CLEANUP_NUM or cleanup_receipt >= MAX_CLEANUP_NUM or cleanup_block >= MAX_CLEANUP_NUM) {
      app().get_io_service().post([=] {
         action a;
         a.name = ACTION_CLEANUP;
         a.data = fc::raw::pack(eoc_icp::cleanup{MAX_CLEANUP_NUM});
         wlog("cleanup *************************************");
         push_transaction(vector<action>{a});
      });
   }

   if (cleanup_packet + cleanup_receipt + cleanup_block < 2*MAX_CLEANUP_NUM) {
      cumulative_cleanup_count_ = 0;
   }
}

/*void relay::cleanup_sequences() {
   ++cumulative_cleanup_sequences_;
   if (cumulative_cleanup_sequences_ < MAX_CLEANUP_SEQUENCES) return;

   auto s = get_read_only_api().get_sequence(true);
   if (not s) {
      elog("got empty sequence");
      return;
   }

   wlog("min_packet_seq: ${m}, last_incoming_receipt_seq: ${r}", ("m", s->min_packet_seq)("r", s->last_incoming_receipt_seq));

   if (s->min_packet_seq > 0 and s->last_incoming_receipt_seq - s->min_packet_seq >= MAX_CLEANUP_SEQUENCES) { // TODO: consistent receipt and packet sequence?
      app().get_io_service().post([=] {
         action a;
         a.name = ACTION_CLEANUP;
         a.data = fc::raw::pack(cleanup{s->min_packet_seq, s->last_incoming_receipt_seq});
         wlog("cleanup: ${s} -> ${e}", ("s", s->min_packet_seq)("e", s->last_incoming_receipt_seq));
         push_transaction(vector<action>{a});
      });
   }
   cumulative_cleanup_sequences_ = 0;
}*/



   //------------------------------------------------------------------------

   void relay::connect( icp_connection_ptr c ) {
      if( c->no_retry != icp_go_away_reason::icp_no_reason) {
         fc_dlog( icp_logger, "Skipping connect due to go_away reason ${r}",("r", reason_str( c->no_retry )));
         return;
      }

      auto colon = c->peer_addr.find(':');

      if (colon == std::string::npos || colon == 0) {
         elog ("Invalid peer address. must be \"host:port\": ${p}", ("p",c->peer_addr));
         for ( auto itr : connections ) {
            if((*itr).peer_addr == c->peer_addr) {
               (*itr).reset();
               close(itr);
               connections.erase(itr);
               break;
            }
         }
         return;
      }

      auto host = c->peer_addr.substr( 0, colon );
      auto port = c->peer_addr.substr( colon + 1);
      idump((host)(port));
      tcp::resolver::query query( tcp::v4(), host.c_str(), port.c_str() );
      icp_connection_wptr weak_conn = c;
      // Note: need to add support for IPv6 too

      resolver->async_resolve( query,
                               [weak_conn, this]( const boost::system::error_code& err,
                                          tcp::resolver::iterator endpoint_itr ){
                                  auto c = weak_conn.lock();
                                  if (!c) return;
                                  if( !err ) {
                                     connect( c, endpoint_itr );
                                  } else {
                                     elog( "Unable to resolve ${peer_addr}: ${error}",
                                           (  "peer_addr", c->peer_name() )("error", err.message() ) );
                                  }
                               });
   }

   void relay::connect( icp_connection_ptr c, tcp::resolver::iterator endpoint_itr ) {
      if( c->no_retry != icp_go_away_reason::icp_no_reason) {
         string rsn = reason_str(c->no_retry);
         return;
      }
      auto current_endpoint = *endpoint_itr;
      ++endpoint_itr;
      c->connecting = true;
      icp_connection_wptr weak_conn = c;
      c->socket->async_connect( current_endpoint, [weak_conn, endpoint_itr, this] ( const boost::system::error_code& err ) {
            auto c = weak_conn.lock();
            if (!c) return;
            if( !err && c->socket->is_open() ) {
               if (start_session( c )) {
                  elog("send->handshake message,start_session");
                  c->send_handshake();
               }
            } else {
               if( endpoint_itr != tcp::resolver::iterator() ) {
                  close(c);
                  connect( c, endpoint_itr );
               }
               else {
                  elog( "connection failed to ${peer}: ${error}",
                        ( "peer", c->peer_name())("error",err.message()));
                  c->connecting = false;

                  close(c);
               }
            }
         } );
   }

   bool relay::start_session( icp_connection_ptr con ) {
      boost::asio::ip::tcp::no_delay nodelay( true );
      boost::system::error_code ec;
      con->socket->set_option( nodelay, ec );
      if (ec) {
         elog( "connection failed to ${peer}: ${error}",
               ( "peer", con->peer_name())("error",ec.message()));
         con->connecting = false;
         close(con);
         return false;
      }
      else {
         start_read_message( con );
         ++started_sessions;
         return true;
         // for now, we can just use the application main loop.
         //     con->readloop_complete  = bf::async( [=](){ read_loop( con ); } );
         //     con->writeloop_complete = bf::async( [=](){ write_loop con ); } );
      }
   }


   void relay::start_listen_loop( ) {
      auto socket = std::make_shared<tcp::socket>( std::ref( app().get_io_service() ) );
      acceptor->async_accept( *socket, [socket,this]( boost::system::error_code ec ) {
            if( !ec ) {
               uint32_t visitors = 0;
               uint32_t from_addr = 0;
               auto paddr = socket->remote_endpoint(ec).address();
               if (ec) {
                  fc_elog(eosio::icp_logger,"Error getting remote endpoint: ${m}",("m", ec.message()));
               }
               else {
                  for (auto &conn : connections) {
                     if(conn->socket->is_open()) {
                        if (conn->peer_addr.empty()) {
                           visitors++;
                           boost::system::error_code ec;
                           if (paddr == conn->socket->remote_endpoint(ec).address()) {
                              from_addr++;
                           }
                        }
                     }
                  }
                  if (num_clients != visitors) {
                     ilog ("checking max client, visitors = ${v} num clients ${n}",("v",visitors)("n",num_clients));
                     num_clients = visitors;
                  }
                  if( from_addr < max_nodes_per_host && (max_client_count == 0 || num_clients < max_client_count )) {
                     ++num_clients;
                     icp_connection_ptr c = std::make_shared<icp_connection>( socket );
                     connections.insert( c );
                     start_session( c );

                  }
                  else {
                     if (from_addr >= max_nodes_per_host) {
                         fc_elog(icp_logger, "Number of connections (${n}) from ${ra} exceeds limit",
                                 ("n", from_addr+1)("ra",paddr.to_string()));
                     }
                     else {
                         fc_elog(icp_logger, "Error max_client_count ${m} exceeded",
                                 ( "m", max_client_count) );
                     }
                     socket->close( );
                  }
               }
            } else {
               elog( "Error accepting connection: ${m}",( "m", ec.message() ) );
               // For the listed error codes below, recall start_listen_loop()
               switch (ec.value()) {
                  case ECONNABORTED:
                  case EMFILE:
                  case ENFILE:
                  case ENOBUFS:
                  case ENOMEM:
                  case EPROTO:
                     break;
                  default:
                     return;
               }
            }
            start_listen_loop();
         });
   }

   void relay::start_read_message( icp_connection_ptr conn ) {

      try {
         if(!conn->socket) {
            return;
         }
         icp_connection_wptr weak_conn = conn;

         std::size_t minimum_read = conn->outstanding_read_bytes ? *conn->outstanding_read_bytes : icp_message_header_size;

         if (use_socket_read_watermark) {
            const size_t max_socket_read_watermark = 4096;
            std::size_t socket_read_watermark = std::min<std::size_t>(minimum_read, max_socket_read_watermark);
            boost::asio::socket_base::receive_low_watermark read_watermark_opt(socket_read_watermark);
            conn->socket->set_option(read_watermark_opt);
         }

         auto completion_handler = [minimum_read](boost::system::error_code ec, std::size_t bytes_transferred) -> std::size_t {
            if (ec || bytes_transferred >= minimum_read ) {
               return 0;
            } else {
               return minimum_read - bytes_transferred;
            }
         };

         boost::asio::async_read(*conn->socket,
            conn->pending_message_buffer.get_buffer_sequence_for_boost_async_read(), completion_handler,
            [this,weak_conn]( boost::system::error_code ec, std::size_t bytes_transferred ) {
               auto conn = weak_conn.lock();
               if (!conn) {
                  return;
               }

               conn->outstanding_read_bytes.reset();

               try {
                  if( !ec ) {
                     if (bytes_transferred > conn->pending_message_buffer.bytes_to_write()) {
                        elog("async_read_some callback: bytes_transfered = ${bt}, buffer.bytes_to_write = ${btw}",
                             ("bt",bytes_transferred)("btw",conn->pending_message_buffer.bytes_to_write()));
                     }
                     EOS_ASSERT(bytes_transferred <= conn->pending_message_buffer.bytes_to_write(), plugin_exception, "");
                     conn->pending_message_buffer.advance_write_ptr(bytes_transferred);
                     while (conn->pending_message_buffer.bytes_to_read() > 0) {
                        uint32_t bytes_in_buffer = conn->pending_message_buffer.bytes_to_read();

                        if (bytes_in_buffer < icp_message_header_size) {
                           conn->outstanding_read_bytes.emplace(icp_message_header_size - bytes_in_buffer);
                           break;
                        } else {
                           uint32_t message_length;
                           auto index = conn->pending_message_buffer.read_index();
                           conn->pending_message_buffer.peek(&message_length, sizeof(message_length), index);
                           if(message_length > icp_def_send_buffer_size*2 || message_length == 0) {
                              boost::system::error_code ec;
                              elog("incoming message length unexpected (${i}), from ${p}", ("i", message_length)("p",boost::lexical_cast<std::string>(conn->socket->remote_endpoint(ec))));
                              close(conn);
                              return;
                           }

                           auto total_message_bytes = message_length + icp_message_header_size;

                           if (bytes_in_buffer >= total_message_bytes) {
                              conn->pending_message_buffer.advance_read_ptr(icp_message_header_size);
                              if (!conn->process_next_message(*this, message_length)) {
                                 return;
                              }
                           } else {
                              auto outstanding_message_bytes = total_message_bytes - bytes_in_buffer;
                              auto available_buffer_bytes = conn->pending_message_buffer.bytes_to_write();
                              if (outstanding_message_bytes > available_buffer_bytes) {
                                 conn->pending_message_buffer.add_space( outstanding_message_bytes - available_buffer_bytes );
                              }

                              conn->outstanding_read_bytes.emplace(outstanding_message_bytes);
                              break;
                           }
                        }
                     }
                     start_read_message(conn);
                  } else {
                     auto pname = conn->peer_name();
                     if (ec.value() != boost::asio::error::eof) {
                        elog( "Error reading message from ${p}: ${m}",("p",pname)( "m", ec.message() ) );
                     } else {
                        ilog( "Peer ${p} closed connection",("p",pname) );
                     }
                     close( conn );
                  }
               }
               catch(const std::exception &ex) {
                  string pname = conn ? conn->peer_name() : "no connection name";
                  elog("Exception in handling read data from ${p} ${s}",("p",pname)("s",ex.what()));
                  close( conn );
               }
               catch(const fc::exception &ex) {
                  string pname = conn ? conn->peer_name() : "no connection name";
                  elog("Exception in handling read data ${s}", ("p",pname)("s",ex.to_string()));
                  close( conn );
               }
               catch (...) {
                  string pname = conn ? conn->peer_name() : "no connection name";
                  elog( "Undefined exception hanlding the read data from connection ${p}",( "p",pname));
                  close( conn );
               }
            } );
      } catch (...) {
         string pname = conn ? conn->peer_name() : "no connection name";
         elog( "Undefined exception handling reading ${p}",("p",pname) );
         close( conn );
      }
   }

   size_t relay::count_open_sockets() const
   {
      size_t count = 0;
      for( auto &c : connections) {
         if(c->socket->is_open())
            ++count;
      }
      return count;
   }

}
