#include "icp_relay.hpp"

#include "api.hpp"
#include "message.hpp"
#include <fc/log/logger.hpp>
#include "icp_sync_manager.hpp"
#include <eosio/eoc_relay_plugin/eoc_relay_plugin.hpp>
#include "icp_connection.hpp"
namespace eoc_icp
{

void dispatch_manager::retry_fetch(icp_connection_ptr conn)
{
}

icp_sync_manager::icp_sync_manager(uint32_t req_span)
    : source(), state(in_sync)
{
   chain_plug = app().find_plugin<chain_plugin>();
   EOS_ASSERT(chain_plug, chain::missing_chain_plugin_exception, "");
}

constexpr auto icp_sync_manager::stage_str(stages s)
{
   switch (s)
   {
   case in_sync:
      return "in sync";
   case lib_catchup:
      return "lib catchup";
   case head_catchup:
      return "head catchup";
   default:
      return "unkown";
   }
}

void icp_sync_manager::set_state(stages newstate)
{
   if (state == newstate)
   {
      return;
   }
   //fc_dlog(logger, "old state ${os} becoming ${ns}",("os",stage_str (state))("ns",stage_str (newstate)));
   state = newstate;
}
//sync required
bool icp_sync_manager::sync_required()
{
}
//receive handshake messag
void icp_sync_manager::recv_handshake(icp_connection_ptr c, const icp_handshake_message &msg)
{
}
} // namespace eoc_icp
