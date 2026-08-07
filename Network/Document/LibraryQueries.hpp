#pragma once
#include <score/model/Identifier.hpp>

#include <score_addon_network_export.h>

namespace score
{
struct DocumentContext;
struct GUIApplicationContext;
}

namespace Network
{
class Client;
class RpcChannel;

/**
 * @brief What the other machine can make, by name.
 *
 * Capabilities already says which process factories a peer has, but only as
 * uuids -- enough to warn, not enough to offer. Someone editing a score that
 * runs elsewhere needs to add the processes *that machine* has, which are
 * exactly the ones missing from the library here.
 *
 * Adding one works because of the layers underneath: the command names a uuid
 * this build cannot make, creation falls back to a stand-in, and the stand-in
 * is then filled from the peer that could make it.
 */
SCORE_ADDON_NETWORK_EXPORT void
bindLibraryQueries(RpcChannel& rpc, const score::DocumentContext& ctx);

//! Ask `peer` for its processes and put the ones this build lacks into the
//! library, so they can be dragged into the score like any other.
SCORE_ADDON_NETWORK_EXPORT void importRemoteLibrary(
    RpcChannel& rpc, const score::GUIApplicationContext& ctx, const Id<Client>& peer);
}
