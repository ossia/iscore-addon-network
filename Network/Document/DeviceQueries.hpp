#pragma once
#include <score_addon_network_export.h>

namespace score
{
struct DocumentContext;
}

namespace Network
{
class RpcChannel;

/**
 * @brief Let a peer ask about the devices of this machine.
 *
 * Devices are the part of a score that is least portable: an ALSA card, a V4L2
 * node, a Syphon server. Which ones exist is a fact about one machine, and a
 * peer editing the score from elsewhere has no way to find out -- a browser has
 * no hardware of its own to enumerate, and the protocol that would do the
 * enumerating may not even be compiled into it.
 *
 * These answer that. They are bound on both ends rather than only on a host,
 * because in a peer session either side may be the one with the camera.
 */
SCORE_ADDON_NETWORK_EXPORT void
bindDeviceQueries(RpcChannel& rpc, const score::DocumentContext& ctx);
}
