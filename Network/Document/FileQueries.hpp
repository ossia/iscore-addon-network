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
 * @brief Let a peer read and write the files this score is made of.
 *
 * A peer editing from elsewhere has no access to the machine the score lives
 * on: it cannot open the sound file a process refers to, list what is in the
 * project folder, or put a file where the score will find it.
 *
 * These speak only in score::Uri, never in paths, and that is the whole of the
 * access control. A Uri names a place relative to the project, the user's
 * library or the media cache; there is no spelling of one that reaches the rest
 * of the filesystem, so a peer cannot ask for /etc/passwd however it phrases
 * the request. Absolute paths are refused rather than resolved.
 */
SCORE_ADDON_NETWORK_EXPORT void
bindFileQueries(RpcChannel& rpc, const score::DocumentContext& ctx);
}
