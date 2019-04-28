#include "icp_relay.hpp"

#include "api.hpp"
#include "message.hpp"
#include <fc/log/logger.hpp>
#include "icp_connection.hpp"
#include <eosio/eoc_relay_plugin/eoc_relay_plugin.hpp>

namespace eoc_icp {

   using icp_connection_ptr = std::shared_ptr<icp_connection>;
   using icp_connection_wptr = std::weak_ptr<icp_connection>;

   using socket_ptr = std::shared_ptr<tcp::socket>;

   using icp_net_message_ptr = std::shared_ptr<icp_net_message>;

   extern  uint16_t icp_net_version_base ;
   extern   uint16_t icp_net_version_range ;
   extern  uint16_t icp_proto_explicit_sync ;
   extern  uint16_t icp_net_version ;

   void
   handshake_initializer::populate(icp_handshake_message &hello)
   {
      hello.network_version = icp_net_version_base + icp_net_version;
      relay *my_impl = app().find_plugin<eoc_relay_plugin>()->get_relay_pointer();
      hello.chain_id = my_impl->chain_id;
      hello.node_id = my_impl->node_id;
      hello.key = my_impl->get_authentication_key();
      hello.time = std::chrono::system_clock::now().time_since_epoch().count();
      hello.token = fc::sha256::hash(hello.time);
      hello.sig = my_impl->sign_compact(hello.key, hello.token);
      // If we couldn't sign, don't send a token.
      if (hello.sig == chain::signature_type())
         hello.token = sha256();
      hello.p2p_address = my_impl->p2p_address + " - " + hello.node_id.str().substr(0, 7);
#if defined(__APPLE__)
      hello.os = "osx";
#elif defined(__linux__)
      hello.os = "linux";
#elif defined(_MSC_VER)
      hello.os = "win32";
#else
      hello.os = "other";
#endif
      hello.agent = my_impl->user_agent_name;

      controller &cc = my_impl->chain_plug->chain();
      hello.head_id = fc::sha256();
      hello.last_irreversible_block_id = fc::sha256();
      hello.head_num = cc.fork_db_head_block_num();
      hello.last_irreversible_block_num = cc.last_irreversible_block_num();
      if (hello.last_irreversible_block_num)
      {
         try
         {
            hello.last_irreversible_block_id = cc.get_block_id_for_num(hello.last_irreversible_block_num);
         }
         catch (const unknown_block_exception &ex)
         {
            ilog("caught unkown_block");
            hello.last_irreversible_block_num = 0;
         }
      }
      if (hello.head_num)
      {
         try
         {
            hello.head_id = cc.get_block_id_for_num(hello.head_num);
         }
         catch (const unknown_block_exception &ex)
         {
            hello.head_num = 0;
         }
      }
   }

   icp_connection::icp_connection(string endpoint)
       : socket(std::make_shared<tcp::socket>(std::ref(app().get_io_service()))),
         node_id(),
         sent_handshake_count(0),
         connecting(false),
         syncing(false),
         protocol_version(0),
         peer_addr(endpoint),
         response_expected(),
         fork_head(),
         fork_head_num(0),
         last_handshake_recv(),
         last_handshake_sent(),
         no_retry(icp_no_reason)
   {
      wlog("created icp_connection to ${n}", ("n", endpoint));
      initialize();
   }

   icp_connection::icp_connection(socket_ptr s)
       : socket(s),
         node_id(),
         sent_handshake_count(0),
         connecting(false),
         syncing(false),
         protocol_version(0),
         peer_addr(),
         response_expected(),
         fork_head(),
         fork_head_num(0),
         last_handshake_recv(),
         last_handshake_sent(),
         no_retry(icp_no_reason)
   {
      wlog("accepted network icp_connection");
      initialize();
   }

   icp_connection::~icp_connection() {}

   void icp_connection::initialize()
   {
      my_impl = app().find_plugin<eoc_relay_plugin>()->get_relay_pointer();
      auto *rnd = node_id.data();
      rnd[0] = 0;
      response_expected.reset(new boost::asio::steady_timer(app().get_io_service()));
   }

   bool icp_connection::connected()
   {
      return (socket && socket->is_open() && !connecting);
   }

   bool icp_connection::current()
   {
      return (connected() && !syncing);
   }

   void icp_connection::reset()
   {
   }

   void icp_connection::flush_queues()
   {
      write_queue.clear();
   }

   void icp_connection::close()
   {
      if (socket)
      {
         socket->close();
      }
      else
      {
         wlog("no socket to close!");
      }
      flush_queues();
      connecting = false;
      syncing = false;

      reset();
      sent_handshake_count = 0;
      //ilog("close connection , count is ${g}",("g",sent_handshake_count));
      //my_impl->sync_master->reset_lib_num(shared_from_this());
      //fc_dlog(logger, "canceling wait on ${p}", ("p",peer_name()));
      cancel_wait();
      pending_message_buffer.reset();
   }

   void icp_connection::stop_send()
   {
      syncing = false;
   }

   const string icp_connection::peer_name()
   {
      if (!last_handshake_recv.p2p_address.empty())
      {
         return last_handshake_recv.p2p_address;
      }
      if (!peer_addr.empty())
      {
         return peer_addr;
      }
      return "connecting client";
   }

   void icp_connection::queue_write(std::shared_ptr<vector<char>> buff,
                                    bool trigger_send,
                                    std::function<void(boost::system::error_code, std::size_t)> callback)
   {
      write_queue.push_back({buff, callback});
      if (out_queue.empty() && trigger_send)
         do_queue_write();
   }
   //process_next_message
   bool icp_connection::process_next_message(relay &impl, uint32_t message_length)
   {
      try
      {
         // If it is a signed_block, then save the raw message for the cache
         // This must be done before we unpack the message.
         // This code is copied from fc::io::unpack(..., unsigned_int)
         auto index = pending_message_buffer.read_index();
         uint64_t which = 0;
         char b = 0;
         uint8_t by = 0;
         do
         {
            pending_message_buffer.peek(&b, 1, index);
            which |= uint32_t(uint8_t(b) & 0x7f) << by;
            by += 7;
         } while (uint8_t(b) & 0x80 && by < 32);

         // if (which == uint64_t(net_message::tag<signed_block>::value)) {
         //    blk_buffer.resize(message_length);
         //    auto index = pending_message_buffer.read_index();
         //    pending_message_buffer.peek(blk_buffer.data(), message_length, index);
         // }
         auto ds = pending_message_buffer.create_datastream();
         icp_net_message msg;
         fc::raw::unpack(ds, msg);
         icp_msgHandler m(impl, shared_from_this());
         msg.visit(m);
      }
      catch (const fc::exception &e)
      {
         edump((e.to_detail_string()));
         impl.close(shared_from_this());
         return false;
      }
      return true;
   }

   void icp_connection::update_local_head(const head &h)
   {
      head_local = h;
   }

   void icp_connection::do_queue_write()
   {
      if (write_queue.empty() || !out_queue.empty())
         return;
      icp_connection_wptr c(shared_from_this());
      if (!socket->is_open())
      {
         // fc_elog(logger,"socket not open to ${p}",("p",peer_name()));
         // my_impl->close(c.lock());
         return;
      }
      std::vector<boost::asio::const_buffer> bufs;
      while (write_queue.size() > 0)
      {
         auto &m = write_queue.front();
         bufs.push_back(boost::asio::buffer(*m.buff));
         out_queue.push_back(m);
         write_queue.pop_front();
      }
      boost::asio::async_write(*socket, bufs, [c,this](boost::system::error_code ec, std::size_t w) {
         try
         {
            auto conn = c.lock();
            if (!conn)
               return;

            for (auto &m : conn->out_queue)
            {
               m.callback(ec, w);
            }

            if (ec)
            {
               string pname = conn ? conn->peer_name() : "no icp_connection name";
               if (ec.value() != boost::asio::error::eof)
               {
                  elog("Error sending to peer ${p}: ${i}", ("p", pname)("i", ec.message()));
               }
               else
               {
                  ilog("icp_connection closure detected on write to ${p}", ("p", pname));
               }
               my_impl->close(conn);
               return;
            }
            while (conn->out_queue.size() > 0)
            {
               conn->out_queue.pop_front();
            }
            //conn->enqueue_sync_block();
            conn->do_queue_write();
         }
         catch (const std::exception &ex)
         {
            auto conn = c.lock();
            string pname = conn ? conn->peer_name() : "no icp_connection name";
            elog("Exception in do_queue_write to ${p} ${s}", ("p", pname)("s", ex.what()));
         }
         catch (const fc::exception &ex)
         {
            auto conn = c.lock();
            string pname = conn ? conn->peer_name() : "no icp_connection name";
            elog("Exception in do_queue_write to ${p} ${s}", ("p", pname)("s", ex.to_string()));
         }
         catch (...)
         {
            auto conn = c.lock();
            string pname = conn ? conn->peer_name() : "no icp_connection name";
            elog("Exception in do_queue_write to ${p}", ("p", pname));
         }
      });
   }

   void icp_connection::send_handshake()
   {
      handshake_initializer::populate(last_handshake_sent);
      last_handshake_sent.generation = ++sent_handshake_count;
      //ilog("after send handshake , count is ${g}",("g",sent_handshake_count));
      ilog("Sending handshake generation ${g} to ${ep}",
           ("g", last_handshake_sent.generation)("ep", peer_name()));
      fc_dlog(icp_logger, "Sending handshake generation ${g} to ${ep}",
              ("g", last_handshake_sent.generation)("ep", peer_name()));
      enqueue(last_handshake_sent);
   }

   void icp_connection::send_icp_net_msg(const icp_net_message &msg)
   {
      enqueue(msg);
   }

   void icp_connection::enqueue(const icp_net_message &m, bool trigger_send)
   {

      icp_go_away_reason close_after_send = icp_no_reason;
      if (m.contains<icp_go_away_message>())
      {
         close_after_send = m.get<icp_go_away_message>().reason;
      }

      uint32_t payload_size = fc::raw::pack_size(m);
      char *header = reinterpret_cast<char *>(&payload_size);
      size_t header_size = sizeof(payload_size);

      size_t buffer_size = header_size + payload_size;

      auto send_buffer = std::make_shared<vector<char>>(buffer_size);
      fc::datastream<char *> ds(send_buffer->data(), buffer_size);
      ds.write(header, header_size);
      fc::raw::pack(ds, m);
      icp_connection_wptr weak_this = shared_from_this();
      auto relay_temp = my_impl;
      queue_write(send_buffer, trigger_send,
                  [weak_this, close_after_send, relay_temp](boost::system::error_code ec, std::size_t) {
                     icp_connection_ptr conn = weak_this.lock();
                     if (conn)
                     {
                        if (close_after_send != icp_no_reason)
                        {
                           elog("sent a go away message: ${r}, closing connection to ${p}", ("r", reason_str(close_after_send))("p", conn->peer_name()));
                           relay_temp->close(conn);
                           return;
                        }
                     }
                     else
                     {
                        fc_wlog(icp_logger, "connection expired before enqueued net_message called callback!");
                     }
                  });
   }

   void icp_connection::cancel_wait()
   {
      if (response_expected)
         response_expected->cancel();
   }

   void icp_connection::sync_wait()
   {
      response_expected->expires_from_now(my_impl->resp_expected_period);
      icp_connection_wptr c(shared_from_this());
      response_expected->async_wait([c](boost::system::error_code ec) {
         icp_connection_ptr conn = c.lock();
         if (!conn)
         {
            // icp_connection was destroyed before this lambda was delivered
            return;
         }

         conn->sync_timeout(ec);
      });
   }

   void icp_connection::fetch_wait()
   {
      // response_expected->expires_from_now( my_impl->resp_expected_period);
      // icp_connection_wptr c(shared_from_this());
      // response_expected->async_wait( [c]( boost::system::error_code ec){
      //       icp_connection_ptr conn = c.lock();
      //       if (!conn) {
      //          // icp_connection was destroyed before this lambda was delivered
      //          return;
      //       }

      //       conn->fetch_timeout(ec);
      //    } );
   }

   void icp_connection::sync_timeout(boost::system::error_code ec)
   {
      // if( !ec ) {
      //    my_impl->sync_master->reassign_fetch (shared_from_this(),benign_other);
      // }
      // else if( ec == boost::asio::error::operation_aborted) {
      // }
      // else {
      //    elog ("setting timer for sync request got error ${ec}",("ec", ec.message()));
      // }
   }

   void icp_connection::fetch_timeout(boost::system::error_code ec)
   {
      // if( !ec ) {
      //    if( pending_fetch.valid() && !( pending_fetch->req_trx.empty( ) || pending_fetch->req_blocks.empty( ) ) ) {
      //       my_impl->dispatcher->retry_fetch (shared_from_this() );
      //    }
      // }
      // else if( ec == boost::asio::error::operation_aborted ) {
      //    if( !connected( ) ) {
      //       fc_dlog(logger, "fetch timeout was cancelled due to dead icp_connection");
      //    }
      // }
      // else {
      //    elog( "setting timer for fetch request got error ${ec}", ("ec", ec.message( ) ) );
      // }
   }
}
