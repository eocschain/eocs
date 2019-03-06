/**
 *  @file

 *  @copyright defined in eos/LICENSE

 */
#pragma once

#include <fc/crypto/sha256.hpp>

struct hello;


namespace eoc_icp{
  class relay;
  struct hello;
  class read_only;
  struct get_info_results;
  class icp_connection;
  struct icp_handshake_message;
}


namespace eosio {

   class net_plugin_impl;
   struct handshake_message;

   struct difchain_message;
   struct connection_impl;
   


   namespace chain_apis {
      class read_only;
   }

namespace chain {

   struct chain_id_type : public fc::sha256 {
      using fc::sha256::sha256;


      chain_id_type() = default;


      template<typename T>
      inline friend T& operator<<( T& ds, const chain_id_type& cid ) {
        ds.write( cid.data(), cid.data_size() );
        return ds;
      }

      template<typename T>
      inline friend T& operator>>( T& ds, chain_id_type& cid ) {
        ds.read( cid.data(), cid.data_size() );
        return ds;
      }


      void reflector_init()const;

      private:


         // Some exceptions are unfortunately necessary:
         template<typename T>
         friend T fc::variant::as()const;

         friend class eosio::chain_apis::read_only;

         friend class eosio::net_plugin_impl;
         friend struct eosio::handshake_message;
         friend struct eosio::difchain_message;
         friend class connection;
         friend class eoc_icp::relay;
         friend struct ::hello; // TODO: Rushed hack to support bnet_plugin. Need a better solution.
         friend struct eoc_icp::hello;
         friend class eoc_icp::read_only;
         friend struct eoc_icp::get_info_results;
         friend class eoc_icp::icp_connection;
         friend struct eoc_icp::icp_handshake_message;

   };

} }  // namespace eosio::chain

namespace fc {
  class variant;
  void to_variant(const eosio::chain::chain_id_type& cid, fc::variant& v);
  void from_variant(const fc::variant& v, eosio::chain::chain_id_type& cid);
} // fc
