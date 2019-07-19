/**
 *  @file

 *  @copyright defined in eos/LICENSE

 */
#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/contract_types.hpp>

namespace eosio { namespace chain {

   class apply_context;

   /**
    * @defgroup native_action_handlers Native Action Handlers
    */
   ///@{
   void apply_lemon_newaccount(apply_context&);
   void apply_lemon_updateauth(apply_context&);
   void apply_lemon_deleteauth(apply_context&);
   void apply_lemon_linkauth(apply_context&);
   void apply_lemon_unlinkauth(apply_context&);

   /*
   void apply_eosio_postrecovery(apply_context&);
   void apply_eosio_passrecovery(apply_context&);
   void apply_eosio_vetorecovery(apply_context&);
   */

   void apply_lemon_setcode(apply_context&);
   void apply_lemon_setabi(apply_context&);

   void apply_lemon_canceldelay(apply_context&);
   ///@}  end action handlers

} } /// namespace eosio::chain
