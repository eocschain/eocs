/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio/chain/types.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/block.hpp>
#include <eosio/net_plugin/protocol.hpp>

#include <fc/network/message_buffer.hpp>
#include <fc/network/ip.hpp>
#include <fc/io/json.hpp>
#include <fc/io/raw.hpp>
#include <fc/log/appender.hpp>
#include <fc/container/flat.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/crypto/rand.hpp>
#include <fc/exception/exception.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/intrusive/set.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/plugin_interface.hpp>
#include <eosio/producer_plugin/producer_plugin.hpp>
#include <regex>
#include <eosio/eoc_relay_plugin/eoc_relay_plugin.hpp>
#include "icp_relay.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy.hpp>
#define MAXSENDNUM 100
namespace fc {
   extern std::unordered_map<std::string,logger>& get_logger_map();
}
namespace eosio {

   using fc::time_point;
   using fc::time_point_sec;
   using signed_block_ptr = std::shared_ptr<eosio::chain::signed_block>;
   using eosio::chain_apis::read_only;
   using eosio::chain_apis::read_write;
   using tcp = boost::asio::ip::tcp;
     using boost::asio::ip::tcp;
   using boost::asio::ip::address_v4;
   using boost::asio::ip::host_name;
   using boost::intrusive::rbtree;
   using boost::multi_index_container;

   constexpr auto     def_send_buffer_size_mb = 4;
   constexpr auto     def_send_buffer_size = 1024*1024*def_send_buffer_size_mb;
   constexpr auto     def_max_clients = 25; // 0 for unlimited clients
   constexpr auto     def_max_nodes_per_host = 1;
   constexpr auto     def_conn_retry_wait = 30;
   constexpr auto     def_txn_expire_wait = std::chrono::seconds(3);
   constexpr auto     def_resp_expected_wait = std::chrono::seconds(5);
   constexpr auto     def_sync_fetch_span = 100;
   constexpr uint32_t  def_max_just_send = 1500; // roughly 1 "mtu"
   constexpr bool     large_msg_notify = false;

   constexpr auto     message_header_size = 4;

 
   /**
    *  If there is a change to network protocol or behavior, increment net version to identify
    *  the need for compatibility hooks
    */
  
    const fc::string icp_logger_name("icp_relay");
     fc::logger icp_logger;
   std::string icp_peer_log_format;

   template<class enum_type, class=typename std::enable_if<std::is_enum<enum_type>::value>::type>
   inline enum_type& operator|=(enum_type& lhs, const enum_type& rhs)
   {
      using T = std::underlying_type_t <enum_type>;
      return lhs = static_cast<enum_type>(static_cast<T>(lhs) | static_cast<T>(rhs));
   }

    template<typename T>
   T icp_dejsonify(const string& s) {
      return fc::json::from_string(s).as<T>();
   }


   static appbase::abstract_plugin& _eoc_relay_plugin = app().register_plugin<eoc_relay_plugin>();
    
   
      eoc_relay_plugin::eoc_relay_plugin(){
      }
      eoc_relay_plugin::~eoc_relay_plugin(){}

      void eoc_relay_plugin::set_program_options(options_description&, options_description& cfg) {
            cfg.add_options()
          ("eoc-relay-endpoint", bpo::value<string>()->default_value("0.0.0.0:8899"), "The endpoint upon which to listen for incoming connections")
          ("eoc-relay-address", bpo::value<string>()->default_value("localhost:8899"), "The localhost addr")
       ("eoc-relay-threads", bpo::value<uint32_t>(), "The number of threads to use to process network messages")
       ("eoc-relay-connect", bpo::value<vector<string>>()->composing(), "Remote endpoint of other node to connect to (may specify multiple times)")
       ("eoc-relay-peer-chain-id", bpo::value<string>()->default_value(""), "The chain id of icp peer")
       ("eoc-relay-peer-contract", bpo::value<string>()->default_value("eocseosioicp"), "The peer icp contract account name")
       ("eoc-relay-local-contract", bpo::value<string>()->default_value("eocseosioicp"), "The local icp contract account name")
       ("eoc-relay-signer", bpo::value<string>()->default_value("eocseosrelay@active"), "The account and permission level to authorize icp transactions on local icp contract, as in 'account@permission'")
        ( "p2p-max-nodes-per-host", bpo::value<int>()->default_value(def_max_nodes_per_host), "Maximum number of client nodes from any single IP address")
        ( "agent-name", bpo::value<string>()->default_value("\"EOS Test Agent\""), "The name supplied to identify this node amongst the peers.")
        ( "icp-allowed-connection", bpo::value<vector<string>>()->multitoken()->default_value({"any"}, "any"), "Can be 'any' or 'producers' or 'specified' or 'none'. If 'specified', peer-key must be specified at least once. If only 'producers', peer-key is not required. 'producers' and 'specified' may be combined.")
        ( "peer-key", bpo::value<vector<string>>()->composing()->multitoken(), "Optional public key of peer allowed to connect.  May be used multiple times.")
        ( "peer-private-key", boost::program_options::value<vector<string>>()->composing()->multitoken(),
           "Tuple of [PublicKey, WIF private key] (may specify multiple times)")
         ( "max-clients", bpo::value<int>()->default_value(def_max_clients), "Maximum number of clients from which connections are accepted, use 0 for no limit")
         ( "connection-cleanup-period", bpo::value<int>()->default_value(def_conn_retry_wait), "number of seconds to wait before cleaning up dead connections")
         ( "max-cleanup-time-msec", bpo::value<int>()->default_value(10), "max connection cleanup time per cleanup call in millisec")
         ( "network-version-match", bpo::value<bool>()->default_value(false),
           "True to require exact match of peer network version.")
         ( "sync-fetch-span", bpo::value<uint32_t>()->default_value(def_sync_fetch_span), "number of blocks to retrieve in a chunk from any individual peer during synchronization")
         ( "max-implicit-request", bpo::value<uint32_t>()->default_value(def_max_just_send), "maximum sizes of transaction or block messages that are sent without first sending a notice")
         ( "use-socket-read-watermark", bpo::value<bool>()->default_value(false), "Enable expirimental socket read watermark optimization")
         ( "peer-log-format", bpo::value<string>()->default_value( "[\"${_name}\" ${_ip}:${_port}]" ),
           "The string used to format peers when logging messages about them.  Variables are escaped with ${<variable name>}.\n"
           "Available Variables:\n"
           "   _name  \tself-reported name\n\n"
           "   _id    \tself-reported ID (64 hex characters)\n\n"
           "   _sid   \tfirst 8 characters of _peer.id\n\n"
           "   _ip    \tremote IP address of peer\n\n"
           "   _port  \tremote port number of peer\n\n"
           "   _lip   \tlocal IP address connected to peer\n\n"
           "   _lport \tlocal port number connected to peer\n\n"),
        
        
        
        
        
        ("relaynodeid",bpo::value< int>()->default_value(0), "relay num begin to send");
          
      }

      vector<chain::permission_level> get_eoc_account_permissions(const vector<string> &permissions)
      {
         auto fixedPermissions = permissions | boost::adaptors::transformed([](const string &p) {
                                    vector<string> pieces;
                                    boost::algorithm::split(pieces, p, boost::algorithm::is_any_of("@"));
                                    if (pieces.size() == 1)
                                       pieces.push_back("active");
                                    return chain::permission_level{.actor = pieces[0], .permission = pieces[1]};
                                 });
         vector<chain::permission_level> accountPermissions;
         boost::range::copy(fixedPermissions, back_inserter(accountPermissions));
         return accountPermissions;
      }

      void eoc_relay_plugin::init_eoc_relay_plugin(const variables_map &options)
      {

         //peer_log_format = options.at( "peer-log-format" ).as<string>();

         relay_->network_version_match = options.at("network-version-match").as<bool>();

         relay_->sync_master.reset(new eoc_icp::icp_sync_manager(options.at("sync-fetch-span").as<uint32_t>()));
         relay_->connector_period = std::chrono::seconds(options.at("connection-cleanup-period").as<int>());
         relay_->max_cleanup_time_ms = options.at("max-cleanup-time-msec").as<int>();
         relay_->txn_exp_period = def_txn_expire_wait;
         relay_->resp_expected_period = def_resp_expected_wait;
         // relay_->dispatcher->just_send_it_max = options.at( "max-implicit-request" ).as<uint32_t>();
         relay_->max_client_count = options.at("max-clients").as<int>();
         relay_->max_nodes_per_host = options.at("p2p-max-nodes-per-host").as<int>();
         relay_->num_clients = 0;
         relay_->started_sessions = 0;

         relay_->use_socket_read_watermark = options.at("use-socket-read-watermark").as<bool>();

         relay_->resolver = std::make_shared<tcp::resolver>(std::ref(app().get_io_service()));

         if (options.count("eoc-relay-endpoint"))
         {
            relay_->p2p_address = options.at("eoc-relay-endpoint").as<string>();
            auto host = relay_->p2p_address.substr(0, relay_->p2p_address.find(':'));
            auto port = relay_->p2p_address.substr(host.size() + 1, relay_->p2p_address.size());
            idump((host)(port));
            tcp::resolver::query query(tcp::v4(), host.c_str(), port.c_str());
            // Note: need to add support for IPv6 too?
            relay_->listen_endpoint = *relay_->resolver->resolve(query);
            relay_->acceptor.reset(new tcp::acceptor(app().get_io_service()));
         }
         else
         {
            if (relay_->listen_endpoint.address().to_v4() == address_v4::any())
            {
               boost::system::error_code ec;
               auto host = host_name(ec);
               if (ec.value() != boost::system::errc::success)
               {

                  FC_THROW_EXCEPTION(fc::invalid_arg_exception,
                                     "Unable to retrieve host_name. ${msg}", ("msg", ec.message()));
               }
               auto port = relay_->p2p_address.substr(relay_->p2p_address.find(':'), relay_->p2p_address.size());
               relay_->p2p_address = host + port;
            }
         }

         auto endpoint = options["eoc-relay-endpoint"].as<string>();
         relay_->endpoint_address_ = endpoint.substr(0, endpoint.find(':'));
         relay_->endpoint_port_ = static_cast<uint16_t>(std::stoul(endpoint.substr(endpoint.find(':') + 1, endpoint.size())));
         ilog("icp_relay_plugin listening on ${host}:${port}", ("host", relay_->endpoint_address_)("port", relay_->endpoint_port_));
      }

      void eoc_relay_plugin::plugin_initialize(const variables_map &options)
      {
         try
         {
            relay_ = std::make_shared<eoc_icp::relay>();
            relay_->chain_plug = app().find_plugin<chain_plugin>();
            EOS_ASSERT(relay_->chain_plug, chain::missing_chain_plugin_exception, "");

            if (options.count("eoc-relay-connect"))
            {
               relay_->connect_to_peers_ = options.at("eoc-relay-connect").as<vector<string>>();
               relay_->supplied_peers = relay_->connect_to_peers_;
            }

            FC_ASSERT(options.count("eoc-relay-peer-chain-id"), "option --icp-relay-peer-chain-id must be specified");
            relay_->local_contract_ = account_name(options.at("eoc-relay-local-contract").as<string>());
            relay_->peer_contract_ = account_name(options.at("eoc-relay-peer-contract").as<string>());
            relay_->peer_chain_id_ = chain_id_type(options.at("eoc-relay-peer-chain-id").as<string>());
            relay_->signer_ = get_eoc_account_permissions(vector<string>{options.at("eoc-relay-signer").as<string>()});
            if (options.count("agent-name"))
            {
               relay_->user_agent_name = options.at("agent-name").as<string>();
            }

            if (options.count("peer-key"))
            {
               const std::vector<std::string> key_strings = options["peer-key"].as<std::vector<std::string>>();
               for (const std::string &key_string : key_strings)
               {
                  relay_->allowed_peers.push_back(icp_dejsonify<chain::public_key_type>(key_string));
               }
            }

            if (options.count("peer-private-key"))
            {
               const std::vector<std::string> key_id_to_wif_pair_strings = options["peer-private-key"].as<std::vector<std::string>>();
               for (const std::string &key_id_to_wif_pair_string : key_id_to_wif_pair_strings)
               {
                  auto key_id_to_wif_pair = icp_dejsonify<std::pair<chain::public_key_type, std::string>>(
                      key_id_to_wif_pair_string);
                  relay_->private_keys[key_id_to_wif_pair.first] = fc::crypto::private_key(key_id_to_wif_pair.second);
               }
            }

            relay_->chain_id = app().get_plugin<chain_plugin>().get_chain_id();
            fc::rand_pseudo_bytes(relay_->node_id.data(), relay_->node_id.data_size());
            ilog("my node_id is ${id}", ("id", relay_->node_id));

            if (options.count("icp-allowed-connection"))
            {
               const std::vector<std::string> allowed_remotes = options["allowed-connection"].as<std::vector<std::string>>();
               for (const std::string &allowed_remote : allowed_remotes)
               {
                  if (allowed_remote == "any")
                     relay_->allowed_connections |= eoc_icp::relay::Any;
                  else if (allowed_remote == "producers")
                     relay_->allowed_connections |= eoc_icp::relay::Producers;
                  else if (allowed_remote == "specified")
                     relay_->allowed_connections |= eoc_icp::relay::Specified;
                  else if (allowed_remote == "none")
                     relay_->allowed_connections = eoc_icp::relay::None;
               }
            }

            init_eoc_relay_plugin(options);
         }

         FC_LOG_AND_RETHROW()
      }

      void eoc_relay_plugin::plugin_startup()
      {
         ilog("starting eoc_relay_plugin");
         //sendstartaction();
         auto &chain = app().get_plugin<chain_plugin>().chain();
         FC_ASSERT(chain.get_read_mode() != chain::db_read_mode::IRREVERSIBLE, "icp is not compatible with \"irreversible\" read_mode");

         relay_->start();
         chain::controller &cc = relay_->chain_plug->chain();
         {
            cc.applied_transaction.connect(boost::bind(&eoc_icp::relay::on_applied_transaction, relay_.get(), _1));
            cc.accepted_block_with_action_digests.connect(boost::bind(&eoc_icp::relay::on_accepted_block, relay_.get(), _1));
            cc.irreversible_block.connect(boost::bind(&eoc_icp::relay::on_irreversible_block, relay_.get(), _1));
         }

         relay_->start_monitors();

         for (auto seed_node : relay_->connect_to_peers_)
         {
            connect(seed_node);
         }

         if (fc::get_logger_map().find(icp_logger_name) != fc::get_logger_map().end())
            icp_logger = fc::get_logger_map()[icp_logger_name];

         // Make the magic happen
      }

      eoc_icp::read_only eoc_relay_plugin::get_read_only_api()
      {
         return relay_->get_read_only_api();
      }
      eoc_icp::read_write eoc_relay_plugin::get_read_write_api()
      {
         return relay_->get_read_write_api();
      }

      void eoc_relay_plugin::plugin_shutdown()
      {
         // OK, that's enough magic
         try
         {
            ilog("shutdown..");
            relay_->done = true;
            if (relay_->acceptor)
            {
               ilog("close acceptor");
               relay_->acceptor->close();

               ilog("close ${s} connections", ("s", relay_->connections.size()));
               auto cons = relay_->connections;
               for (auto con : cons)
               {
                  relay_->close(con);
               }

               relay_->acceptor.reset(nullptr);
            }
            ilog("exit shutdown");
         }
         FC_CAPTURE_AND_RETHROW()
      }

      string eoc_relay_plugin::connect(const string &host)
      {
         if (relay_->find_connection(host))
            return "already connected";

         eoc_icp::icp_connection_ptr c = std::make_shared<eoc_icp::icp_connection>(host);
         //fc_dlog(logger,"adding new connection to the list");
         relay_->connections.insert(c);
         //fc_dlog(logger,"calling active connector");
         relay_->connect(c);
         return "added connection";
      }

      eoc_icp::relay *eoc_relay_plugin::get_relay_pointer()
      {
         return relay_.get();
      }
}
