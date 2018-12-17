/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/multi_index_container.hpp>
#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/block.hpp>
#include <httpc.hpp>
#include <localize.hpp>
namespace eosio {
  class connection;
  using connection_ptr = std::shared_ptr<connection>;
  //signed_block plus version
  struct signed_blockplus
  {
      public:
      uint32_t m_nNum;
      eosio::chain::signed_block m_block;   
  };

    extern eosio::client::http::http_context context;
    struct indexblocknum{};
    typedef boost::multi_index::multi_index_container<
        signed_blockplus,
        indexed_by<
        ordered_unique<tag<indexblocknum>,member<signed_blockplus, uint32_t, &signed_blockplus::m_nNum> >
        > >blockcontainer;

   class net_difchain_plugin 
   {
      public:
        
        virtual ~net_difchain_plugin(){}
        static boost::shared_ptr<net_difchain_plugin> instance(){
            if(m_pSelf == 0)
            {
                m_pSelf = boost::shared_ptr<net_difchain_plugin>(new net_difchain_plugin() );
                context = eosio::client::http::create_http_context();
            }              
            return m_pSelf;
        }

      uint32_t getNumSend(); 
      void analyDifChainMsg(std::vector<eosio::chain::signed_block_ptr> & vecBlocks);
      void recvDifChainMsg(const difchain_message &msg);
      void getInfo();
      fc::variant json_from_file_or_string(const string& file_or_str, fc::json::parse_type ptype = fc::json::legacy_parser);
      void transaction_next(const fc::static_variant<fc::exception_ptr, eosio::chain::transaction_trace_ptr>& param);
      void send_actions(std::vector<chain::action>&& actions, int32_t extra_kcpu = 1000, packed_transaction::compression_type compression = packed_transaction::none );
      fc::variant determine_required_keys(const signed_transaction& trx) ;
      void sign_transaction(signed_transaction& trx, fc::variant& required_keys, const chain_id_type& chain_id);
      bytes variant_to_bin( const account_name& account, const action_name& action, const fc::variant& action_args_var ) ;
      private:
      net_difchain_plugin(){m_nNumSend = 1;}
      net_difchain_plugin(const net_difchain_plugin& plugin){m_nNumSend = 1;}
      net_difchain_plugin& operator = (const net_difchain_plugin&) {return * this;}
      static  boost::shared_ptr<net_difchain_plugin> m_pSelf;
      blockcontainer m_container;
      uint32_t m_nNumSend;
  };
}