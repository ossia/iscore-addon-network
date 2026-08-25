#pragma once
#include <score_addon_network_export.h>

class QObject;
namespace score
{
struct DocumentContext;
}

namespace Network
{
class Session;

/**
 * @brief What a device turned out to contain.
 *
 * A command that adds a device carries the node as it was when it was made, and
 * every peer then refreshes and finds the children itself. One that builds no
 * device -- a terminal -- cannot, so it keeps an empty device. The same goes for
 * anything discovered later, by OSCQuery or by learning.
 *
 * So the machine that has the device sends its tree, rather than the one that
 * does not asking for it.
 */
SCORE_ADDON_NETWORK_EXPORT void
bindDeviceTreeBroadcast(QObject& owner, Session&, const score::DocumentContext&);

//! The other end: put the reported tree in place of ours.
SCORE_ADDON_NETWORK_EXPORT void
bindDeviceTreeMirror(QObject& owner, Session&, const score::DocumentContext&);
}
