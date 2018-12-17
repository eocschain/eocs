/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/block.hpp>
#include <eosio/chain/types.hpp>
#include <chrono>
#include <fc/static_variant.hpp>
#include "message.hpp"

namespace eosio {
   using namespace chain;
   using namespace fc;
}

namespace eoc_icp{
   static_assert(sizeof(std::chrono::system_clock::duration::rep) >= 8, "system_clock is expected to be at least 64 bits");
   typedef std::chrono::system_clock::duration::rep tstamp;

   struct icp_handshake_message{
       uint16_t                   network_version = 0; ///< incremental value above a computed base
      eosio::chain::chain_id_type              chain_id; ///< used to identify chain
      fc::sha256                 node_id; ///< used to identify peers and prevent self-connect
      eosio::chain::public_key_type     key; ///< authentication key; may be a producer or peer key, or empty
      tstamp                     time;
      fc::sha256                 token; ///< digest of time to prove we own the private key of the key above
      eosio::chain::signature_type      sig; ///< signature for the digest
      string                     p2p_address;
      uint32_t                   last_irreversible_block_num = 0;
      eosio::chain::block_id_type              last_irreversible_block_id;
      uint32_t                   head_num = 0;
       eosio::chain::block_id_type              head_id;
      string                     os;
      string                     agent;
      int16_t                    generation;
   };


   enum icp_go_away_reason {
    icp_no_reason, ///< no reason to go away
    icp_self, ///< the connection is to itself
    icp_duplicate, ///< the connection is redundant
    icp_wrong_chain, ///< the peer's chain id doesn't match
    icp_wrong_version, ///< the peer's network version doesn't match
    icp_forked, ///< the peer's irreversible blocks are different
    icp_unlinkable, ///< the peer sent a block we couldn't use
    icp_bad_transaction, ///< the peer sent a transaction that failed verification
    icp_validation, ///< the peer sent a block that failed validation
    icp_benign_other, ///< reasons such as a timeout. not fatal but warrant resetting
    icp_fatal_other, ///< a catch-all for errors we don't have discriminated
    icp_authentication, ///< peer failed authenicatio
    icp_different_chainid
  };

   constexpr auto reason_str( icp_go_away_reason rsn ) {
    switch (rsn ) {
    case icp_no_reason : return "no reason";
    case icp_self : return "self connect";
    case icp_duplicate : return "duplicate";
    case icp_wrong_chain : return "wrong chain";
    case icp_wrong_version : return "wrong version";
    case icp_forked : return "chain is forked";
    case icp_unlinkable : return "unlinkable block received";
    case icp_bad_transaction : return "bad transaction";
    case icp_validation : return "invalid block";
    case icp_authentication : return "authentication failure";
    case icp_fatal_other : return "some other failure";
    case icp_benign_other : return "some other non-fatal condition";
    default : return "some crazy reason";
    }
  }

   struct icp_notice_message{
      head local_head_;

   };

   struct icp_go_away_message {
    icp_go_away_message (icp_go_away_reason r = icp_no_reason) : reason(r), node_id() {}
    icp_go_away_reason reason;
    fc::sha256 node_id; ///< for duplicate notification
  };

   using icp_net_message = fc::static_variant<icp_handshake_message,
   icp_go_away_message,
   icp_notice_message,
    channel_seed,
   head_notice,
   block_header_with_merkle_path,
   icp_actions,
   packet_receipt_request>;

} // namespace eosio


FC_REFLECT( eoc_icp::icp_handshake_message,
            (network_version)(chain_id)(node_id)(key)
            (time)(token)(sig)(p2p_address)
            (last_irreversible_block_num)(last_irreversible_block_id)
            (head_num)(head_id)
            (os)(agent)(generation) )

FC_REFLECT( eoc_icp::icp_go_away_message,
            (reason)(node_id) )

FC_REFLECT( eoc_icp::icp_notice_message,(local_head_) )
/**
 *
Goals of Network Code
1. low latency to minimize missed blocks and potentially reduce block interval
2. minimize redundant data between blocks and transactions.
3. enable rapid sync of a new node
4. update to new boost / fc



State:
   All nodes know which blocks and transactions they have
   All nodes know which blocks and transactions their peers have
   A node knows which blocks and transactions it has requested
   All nodes know when they learned of a transaction

   send hello message
   write loop (true)
      if peer knows the last irreversible block {
         if peer does not know you know a block or transactions
            send the ids you know (so they don't send it to you)
            yield continue
         if peer does not know about a block
            send transactions in block peer doesn't know then send block summary
            yield continue
         if peer does not know about new public endpoints that you have verified
            relay new endpoints to peer
            yield continue
         if peer does not know about transactions
            sends the oldest transactions that is not known by the remote peer
            yield continue
         wait for new validated block, transaction, or peer signal from network fiber
      } else {
         we assume peer is in sync mode in which case it is operating on a
         request / response basis

         wait for notice of sync from the read loop
      }


    read loop
      if hello message
         verify that peers Last Ir Block is in our state or disconnect, they are on fork
         verify peer network protocol

      if notice message update list of transactions known by remote peer
      if trx message then insert into global state as unvalidated
      if blk summary message then insert into global state *if* we know of all dependent transactions
         else close connection


    if my head block < the LIB of a peer and my head block age > block interval * round_size/2 then
    enter sync mode...
        divide the block numbers you need to fetch among peers and send fetch request
        if peer does not respond to request in a timely manner then make request to another peer
        ensure that there is a constant queue of requests in flight and everytime a request is filled
        send of another request.

     Once you have caught up to all peers, notify all peers of your head block so they know that you
     know the LIB and will start sending you real time transactions

parallel fetches, request in groups


only relay transactions to peers if we don't already know about it.

send a notification rather than a transaction if the txn is > 3mtu size.





*/
