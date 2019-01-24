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
#include <fc/io/json.hpp>
#include <api.hpp>
namespace eoc_icp {
   class relay; // forward declaration
   
}
namespace eosio {
  using namespace chain;
 
    class eoc_relay_plugin;
 
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
   
   void get_info();
     
     
   string                       connect( const string& endpoint );
   eoc_icp::relay* get_relay_pointer();
    eoc_icp::read_only get_read_only_api();
   eoc_icp::read_write get_read_write_api();

private:
  
   std::shared_ptr<class eoc_icp::relay> relay_;
};


}
