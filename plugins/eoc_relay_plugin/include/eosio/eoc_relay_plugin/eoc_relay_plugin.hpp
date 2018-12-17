/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/multi_index_container.hpp>
#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/block.hpp>
#include <httpc.hpp>
#include <localize.hpp>
#include <fc/io/json.hpp>
#include <api.hpp>
namespace eoc_icp {
   class relay; // forward declaration
   
}
namespace eosio {
  using namespace chain;
 
    class eoc_relay_plugin;
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
 
/**
 *  This is a template plugin, intended to serve as a starting point for making new plugins
 */
class eoc_relay_plugin : public appbase::plugin<eoc_relay_plugin> {
public:
   eoc_relay_plugin();
   virtual ~eoc_relay_plugin();
 
   APPBASE_PLUGIN_REQUIRES()
   virtual void set_program_options(options_description&, options_description& cfg) override;
 
   void plugin_initialize(const variables_map& options);
   void init_eoc_relay_plugin(const variables_map&options);
   void plugin_startup();
   void plugin_shutdown();
   uint32_t get_num_send(); 
   void analy_dif_chainmsg(std::vector<eosio::chain::signed_block_ptr> & vecBlocks);
   void get_info();
   void get_relay_info();
   void sendrelayaction();
   void sendstartaction();
    fc::variant json_from_file_or_string(const string& file_or_str, fc::json::parse_type ptype = fc::json::legacy_parser);
      void transaction_next(const fc::static_variant<fc::exception_ptr, eosio::chain::transaction_trace_ptr>& param);
      void send_actions(std::vector<chain::action>&& actions, int32_t extra_kcpu = 1000, packed_transaction::compression_type compression = packed_transaction::none );
      fc::variant determine_required_keys(const signed_transaction& trx) ;
      void sign_transaction(signed_transaction& trx, fc::variant& required_keys, const chain_id_type& chain_id);
      bytes variant_to_bin( const account_name& account, const action_name& action, const fc::variant& action_args_var ) ;
      blockcontainer m_container;
      uint64_t m_nblockno;
      uint64_t m_nnodeid;
   string                       connect( const string& endpoint );
   eoc_icp::relay* get_relay_pointer();
    eoc_icp::read_only get_read_only_api();
   eoc_icp::read_write get_read_write_api();

private:
  
   std::shared_ptr<class eoc_icp::relay> relay_;
};


}
