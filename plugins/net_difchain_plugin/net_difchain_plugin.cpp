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
#include <eosio/net_difchain_plugin/net_difchain_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/plugin_interface.hpp>
#include <eosio/producer_plugin/producer_plugin.hpp>
#include <regex>

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

    string url = "http://47.105.111.1:9991/";
    string wallet_url = "http://47.105.111.1:55553/";
    eosio::client::http::http_context context;
    bool no_verify = false;
    vector<string> headers;
    auto   tx_expiration = fc::seconds(30);
    const fc::microseconds abi_serializer_max_time = fc::seconds(10); // No risk to client side serialization taking a long time
    string tx_ref_block_num_or_id;
    bool   tx_force_unique = false;
    bool tx_dont_broadcast = false;
    bool tx_return_packed = false;
    bool tx_skip_sign = false;
    bool tx_print_json = false;
    bool print_request = false;
    bool print_response = false;

    uint8_t tx_max_cpu_usage = 0;
    uint32_t tx_max_net_usage = 0;

    vector<string> tx_permission;

    template<typename T>
    fc::variant call( const std::string& url,
                  const std::string& path,
                  const T& v ) {
   try {
      auto sp = std::make_unique<eosio::client::http::connection_param>(context, client::http::parse_url(url) + path, no_verify ? false : true, headers);
      return eosio::client::http::do_http_call(*sp, fc::variant(v), print_request, print_response );
    }
   catch(boost::system::system_error& e) {
      if(url == eosio::url)
         std::cerr << client::localize::localized("Failed to connect to nodeos at ${u}; is nodeos running?", ("u", url)) << std::endl;
      else if(url == eosio::wallet_url)
         std::cerr << client::localize::localized("Failed to connect to keosd at ${u}; is keosd running?", ("u", url)) << std::endl;
      throw client::http::connection_exception(fc::log_messages{FC_LOG_MESSAGE(error, e.what())});
        }
    }


    template<typename T>
    fc::variant call( const std::string& path,
                  const T& v ) { return call( url, path, fc::variant(v) ); }

    template<>
    fc::variant call( const std::string& url,
                  const std::string& path) { return call( url, path, fc::variant() ); }


   auto abi_serializer_resolver = [](const name& account) -> optional<abi_serializer> {
   static unordered_map<account_name, optional<abi_serializer> > abi_cache;
   auto it = abi_cache.find( account );
   if ( it == abi_cache.end() ) {
      eosio::chain_apis::read_only::get_abi_params abi_param;
      abi_param.account_name= account;
      eosio::chain_apis::read_only readonlydata = app().get_plugin<chain_plugin>().get_read_only_api();
      eosio::chain_apis::read_only::get_abi_results  abi_results=readonlydata.get_abi( abi_param );
      optional<abi_serializer> abis;
      if( abi_results.abi.valid() ) {
         abis.emplace( *abi_results.abi, abi_serializer_max_time );
      } else {
         std::cerr << "ABI for contract " << account.to_string() << " not found. Action data will be shown in hex only." << std::endl;
      }
      abi_cache.emplace( account, abis );

      return abis;
   }

   return it->second;
    };


   
   boost::shared_ptr<net_difchain_plugin> net_difchain_plugin::m_pSelf = NULL;
   uint32_t net_difchain_plugin::getNumSend()
   {
       return m_nNumSend;
   }

   void net_difchain_plugin::analyDifChainMsg(std::vector<chain::signed_block_ptr> & vecBlocks)
   {
      // ilog("begin analyDifChainMsg-------------------------");
       controller& cc = appbase::app().find_plugin<chain_plugin>()->chain();
       uint32_t lastnum = cc.last_irreversible_block_num();
       uint32_t headnum = m_nNumSend;
       //ilog("begin head num is ${headnum}",("headnum", m_nNumSend));
       if(headnum > lastnum)
            return;
       if(lastnum > headnum+MAXSENDNUM)
            lastnum = headnum+MAXSENDNUM;
       for(uint32_t i = headnum; i <= lastnum; i++)
       {
           signed_block_ptr blockptr = cc.fetch_block_by_number(i);
           vecBlocks.push_back(blockptr);
           //ilog("block num is : ${blocknum}",("blocknum", blockptr->block_num()));
       }    
       m_nNumSend = lastnum+1;
       //ilog("end head num is ${headnum}",("headnum", m_nNumSend));
   }

   void net_difchain_plugin::recvDifChainMsg(const difchain_message &msg)
   {
       // ilog("begin recvDifChainMsg-------------------------");
         ilog( "message network_version is ${network_version}", ("network_version", msg.network_version));
         ilog( "message chainid is ${chainid}",("chainid",msg.chain_id));
         ilog( "message blocknum is ${blocknum}",("blocknum",msg.signedblock.block_num()));
        // ilog("message blockid is ${blockid}",("blockid",msg.signedblock.id()));
        // ilog("message blockadder is ${blockadder}",("blockadder",msg.blockadder));
        uint32_t blocknum = msg.signedblock.block_num();
        blockcontainer::index<indexblocknum>::type& indexOfBlock = m_container.get<indexblocknum>();
        auto iterfind = indexOfBlock.find(blocknum);
        //find block num
        if(iterfind != indexOfBlock.end())
        {
            ilog( "blocknum  ${blocknum} has exist",("blocknum",blocknum));
            return;
        }
        //indexofblock empty
        if(indexOfBlock.empty())
        {
            signed_blockplus blockplus;
            blockplus.m_nNum = blocknum;
            blockplus.m_block = msg.signedblock;
            m_container.insert(blockplus);
            ilog( "insert blocknum ${blocknum} success ",("blocknum",blocknum));
            return;
        }
        auto iterMax = indexOfBlock.end();
        iterMax--;
        //current block is smaller than the max block in multi_index container
        if(iterMax->m_nNum > blocknum)
        {
            ilog( "blocknum  ${blocknum} smaller than latest",("blocknum",blocknum));
            return;
        }  
        signed_blockplus blockplus;
        blockplus.m_nNum = blocknum;
        blockplus.m_block = msg.signedblock;
        m_container.insert(blockplus);
        ilog( "insert blocknum ${blocknum} success ",("blocknum",blocknum));
       // ilog("recvDifChainMsg success-------------------------");
   }

    
    fc::variant net_difchain_plugin::json_from_file_or_string(const string& file_or_str, fc::json::parse_type ptype )
    {
        std::regex r("^[ \t]*[\{\[]");
        if ( !std::regex_search(file_or_str, r) && fc::is_regular_file(file_or_str) ) {
            return fc::json::from_file(file_or_str, ptype);
            } else {
      return fc::json::from_string(file_or_str, ptype);
        }
    }

    void net_difchain_plugin::transaction_next(const fc::static_variant<fc::exception_ptr, eosio::chain::transaction_trace_ptr>& param)
    {
        ilog("transaction_next call success !!!!");
    }

   void net_difchain_plugin::getInfo()
   { 
     eosio::chain_apis::read_only readonlydata = app().get_plugin<chain_plugin>().get_read_only_api();
     eosio::chain_apis::read_only::get_table_rows_params tableparam;
     //curl  http://127.0.0.1:8888/v1/chain/get_table_rows -X POST -d '{"scope":"inita", "code":"currency", "table":"account", "json": true}'
     try{
        tableparam.code="eosio.token";
         tableparam.table="accounts";
        tableparam.scope="twinkle11111";
        tableparam.json = true;
        read_only::get_table_rows_result  tableres= readonlydata.get_table_rows(tableparam);
        //ilog( "insert tableresrow size  ${tablerowsize} success ",("tablerowsize",tableres.rows.size() ) );
        for(unsigned int i =0; i < tableres.rows.size(); i++)
        {
            ilog("row index is ${index}",("index",i));
            ilog( "rows data is ${rowsdata} ",("rowsdata",tableres.rows[0]) );
        }

     }
     catch(const fc::exception& e)
     {
         edump((e.to_detail_string() ));
     }
     
     //1 
    //  eosio::chain_apis::read_write readwritedata = app().get_plugin<chain_plugin>().get_read_write_api();
    //  read_write::push_transaction_params params;
    //  eosio::chain::plugin_interface::next_function<read_write::push_transaction_results> next;
    //  readwritedata.push_transaction(params, next);
   
    const std::string fromid="\"eosio\"";
    const std::string toid = "\"twinkle11111\"";
    const std::string quantity="\"1.0000 SYS\"";
    const std::string data="{\"from\":"+fromid+","+"\"to\":"+toid+","+
    "\"quantity\":"+quantity+","+"\"memo\":test}";
    
    ilog("action data is ${actiondata}",("actiondata",data));
    fc::variant action_args_var;
    action_args_var = json_from_file_or_string(data, fc::json::relaxed_parser);
    std::vector<eosio::chain::permission_level> auth;
    eosio::chain::permission_level plv;
    plv.permission= "active";
    plv.actor = "eosio";
    auth.push_back(plv);
    eosio::chain::account_name contract_account("eosio.token");
    eosio::chain::action_name action("transfer");
    eosio::chain::bytes databytes = variant_to_bin( contract_account, action, action_args_var );
    if(databytes.empty())
    {
        ilog("databytes is empty???????????????????????");
        return;
    }
    send_actions({chain::action{auth, contract_account, action, databytes }});
   }


     eosio::chain::bytes eosio::net_difchain_plugin::variant_to_bin( const account_name& account, const action_name& action, const fc::variant& action_args_var ) {
        eosio::chain::bytes databytes;
        try{  
            auto abis = abi_serializer_resolver( account );
            if(abis.valid() == false )
                return databytes; 
            auto action_type = abis->get_action_type( action );
            if(action_type.empty() )
                return databytes;
            return abis->variant_to_binary( action_type, action_args_var, abi_serializer_max_time );

        }
        catch(const fc::exception& e)
        {
            edump((e.to_detail_string() ));
            return databytes;
        }
        catch(const boost::exception& e)
        {
            ilog("boost excpetion is ${exception}",("exception",boost::diagnostic_information(e).c_str()));
            return databytes;
        }
        
     }

    fc::variant net_difchain_plugin::determine_required_keys(const signed_transaction& trx)
    {
         const auto& public_keys = call(wallet_url, eosio::client::http::wallet_public_keys);
            auto get_arg = fc::mutable_variant_object
           ("transaction", (transaction)trx)
           ("available_keys", public_keys);
            const auto& required_keys = call(eosio::client::http::get_required_keys, get_arg);
            return required_keys["required_keys"];
    }

    void net_difchain_plugin::sign_transaction(signed_transaction& trx, fc::variant& required_keys, const chain_id_type& chain_id) {
            fc::variants sign_args = {fc::variant(trx), required_keys, fc::variant(chain_id)};
            const auto& signed_trx = call(wallet_url, eosio::client::http::wallet_sign_trx, sign_args);
            trx = signed_trx.as<signed_transaction>();
    }


   void net_difchain_plugin::send_actions(std::vector<chain::action>&& actions, int32_t extra_kcpu, packed_transaction::compression_type compression  )
    {
        try{
            eosio::chain::packed_transaction_ptr packed_trx=std::make_shared<eosio::chain::packed_transaction>();
        eosio::chain::plugin_interface::next_function<eosio::chain::transaction_trace_ptr> next = std::bind(&net_difchain_plugin::transaction_next,
         this,std::placeholders::_1);

        signed_transaction trx;
        trx.actions = std::forward<decltype(actions)>(actions);
        read_only::get_info_params infoparams;
        eosio::chain_apis::read_only readonlydata = app().get_plugin<chain_plugin>().get_read_only_api();
        read_only::get_info_results infores = readonlydata.get_info(infoparams);
        auto   tx_expiration = fc::seconds(30);
        trx.expiration = infores.head_block_time + tx_expiration;
        block_id_type ref_block_id = infores.last_irreversible_block_id;
        trx.set_reference_block(ref_block_id);
        uint8_t  tx_max_cpu_usage = 0;
        uint32_t tx_max_net_usage = 0;
        trx.max_cpu_usage_ms = tx_max_cpu_usage;
        trx.max_net_usage_words = (tx_max_net_usage + 7)/8;
        if (!tx_skip_sign) {
                 auto required_keys = determine_required_keys(trx);
                 sign_transaction(trx, required_keys, infores.chain_id);
             }
        ilog("begin packed  -----------------------");
        *packed_trx= packed_transaction(trx, compression);
         ilog("packed success !!!!!!!!!!!!!!!!!!!");
        app().get_plugin<producer_plugin>().process_transaction(packed_trx, true, next);

        }catch(const fc::exception& e)
        {
            edump((e.to_detail_string() ));
        }
        
    }
    
}
   

   


