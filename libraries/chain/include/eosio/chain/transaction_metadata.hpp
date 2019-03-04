/**
 *  @file
<<<<<<< HEAD
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/block.hpp>
#include <eosio/chain/trace.hpp>

namespace eosio { namespace chain {

=======
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/types.hpp>
#include <future>

namespace boost { namespace asio {
   class thread_pool;
}}

namespace eosio { namespace chain {

class transaction_metadata;
using transaction_metadata_ptr = std::shared_ptr<transaction_metadata>;
>>>>>>> otherb
/**
 *  This data structure should store context-free cached data about a transaction such as
 *  packed/unpacked/compressed and recovered keys
 */
class transaction_metadata {
   public:
      transaction_id_type                                        id;
      transaction_id_type                                        signed_id;
<<<<<<< HEAD
      signed_transaction                                         trx;
      packed_transaction                                         packed_trx;
      optional<pair<chain_id_type, flat_set<public_key_type>>>   signing_keys;
=======
      packed_transaction_ptr                                     packed_trx;
      fc::microseconds                                           sig_cpu_usage;
      optional<pair<chain_id_type, flat_set<public_key_type>>>   signing_keys;
      std::future<std::tuple<chain_id_type, fc::microseconds, flat_set<public_key_type>>>
                                                                 signing_keys_future;
>>>>>>> otherb
      bool                                                       accepted = false;
      bool                                                       implicit = false;
      bool                                                       scheduled = false;

<<<<<<< HEAD
      explicit transaction_metadata( const signed_transaction& t, packed_transaction::compression_type c = packed_transaction::none )
      :trx(t),packed_trx(t, c) {
         id = trx.id();
         //raw_packed = fc::raw::pack( static_cast<const transaction&>(trx) );
         signed_id = digest_type::hash(packed_trx);
      }

      explicit transaction_metadata( const packed_transaction& ptrx )
      :trx( ptrx.get_signed_transaction() ), packed_trx(ptrx) {
         id = trx.id();
         //raw_packed = fc::raw::pack( static_cast<const transaction&>(trx) );
         signed_id = digest_type::hash(packed_trx);
      }

      const flat_set<public_key_type>& recover_keys( const chain_id_type& chain_id ) {
         if( !signing_keys || signing_keys->first != chain_id ) // Unlikely for more than one chain_id to be used in one nodeos instance
            signing_keys = std::make_pair( chain_id, trx.get_signature_keys( chain_id ) );
         return signing_keys->second;
      }

      uint32_t total_actions()const { return trx.context_free_actions.size() + trx.actions.size(); }
};

using transaction_metadata_ptr = std::shared_ptr<transaction_metadata>;
=======
      transaction_metadata() = delete;
      transaction_metadata(const transaction_metadata&) = delete;
      transaction_metadata(transaction_metadata&&) = delete;
      transaction_metadata operator=(transaction_metadata&) = delete;
      transaction_metadata operator=(transaction_metadata&&) = delete;

      explicit transaction_metadata( const signed_transaction& t, packed_transaction::compression_type c = packed_transaction::none )
      :id(t.id()), packed_trx(std::make_shared<packed_transaction>(t, c)) {
         //raw_packed = fc::raw::pack( static_cast<const transaction&>(trx) );
         signed_id = digest_type::hash(*packed_trx);
      }

      explicit transaction_metadata( const packed_transaction_ptr& ptrx )
      :id(ptrx->id()), packed_trx(ptrx) {
         //raw_packed = fc::raw::pack( static_cast<const transaction&>(trx) );
         signed_id = digest_type::hash(*packed_trx);
      }

      const flat_set<public_key_type>& recover_keys( const chain_id_type& chain_id );

      static void create_signing_keys_future( const transaction_metadata_ptr& mtrx, boost::asio::thread_pool& thread_pool,
                                              const chain_id_type& chain_id, fc::microseconds time_limit );

};
>>>>>>> otherb

} } // eosio::chain
