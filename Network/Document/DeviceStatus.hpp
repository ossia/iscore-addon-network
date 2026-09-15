#pragma once
#include <score/model/Identifier.hpp>

#include <score_addon_network_export.h>

class QObject;
namespace score
{
struct DocumentContext;
}

namespace Network
{
class Client;
class RpcChannel;
class Session;

/**
 * @brief Whether the score's devices are connected, from the machine that has
 * them.
 *
 * A peer that does not run the score has no DeviceInterface behind any of its
 * device nodes, so asking one would report everything as disconnected. That is
 * not "unknown", it is wrong: the devices are connected, just not here.
 *
 * The host reports each device as it connects or disconnects, and once for
 * everything when a peer joins, so a terminal that arrives mid-session is not
 * left with a blank picture until something happens to change.
 */
SCORE_ADDON_NETWORK_EXPORT void
bindDeviceStatusBroadcast(QObject& owner, Session&, const score::DocumentContext&);

//! The other end: apply what the host reports.
SCORE_ADDON_NETWORK_EXPORT void
bindDeviceStatusMirror(QObject& owner, Session&, const score::DocumentContext&);

//! Send the state of every device now, for a peer that has just joined.
SCORE_ADDON_NETWORK_EXPORT void
broadcastAllDeviceStatus(Session&, const score::DocumentContext&);

//! Offer device.statuses, so a peer can ask rather than hope: a push at join
//! races the joiner, which registers handlers only once the document is in.
SCORE_ADDON_NETWORK_EXPORT void
bindDeviceStatusQuery(RpcChannel& rpc, const score::DocumentContext& ctx);

//! Ask `peer` for the state of all its devices.
SCORE_ADDON_NETWORK_EXPORT void requestDeviceStatus(
    RpcChannel& rpc, const score::DocumentContext& ctx, const Id<Client>& peer);
}
