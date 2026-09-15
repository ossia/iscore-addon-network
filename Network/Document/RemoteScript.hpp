#pragma once
#include <score_addon_network_export.h>

namespace score
{
struct DocumentContext;
}

namespace Network
{
class RpcChannel;
class ClientSession;

/**
 * @brief A script typed on a terminal runs on the machine with the devices.
 *
 * Commands happen to replicate, so document edits appear to work from a
 * terminal's console. Nothing else does: `Score.device("x")` looks in a device
 * list that is empty here and always will be, execution is not running here,
 * and the hardware is elsewhere. Forwarding one call at a time would mean
 * maintaining a second, partial API; the script goes over instead.
 */
SCORE_ADDON_NETWORK_EXPORT void
bindScriptForwarding(ClientSession& session, const score::DocumentContext&);

//! The other end: run it here, and send back what a console would have shown.
SCORE_ADDON_NETWORK_EXPORT void
bindScriptEvaluation(RpcChannel& rpc, const score::DocumentContext&);
}
