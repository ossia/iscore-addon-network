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
 * @brief Where the score has got to, from the machine running it.
 *
 * A terminal has no executor, so nothing moves its playhead. Yet the position
 * is most of what someone watching a remote score is watching, and it costs
 * one number: IntervalDurations::setPlayPercentage is a plain model setter and
 * the executor is merely one of its callers.
 *
 * Only the root interval. Nested intervals get their positions from their own
 * executor components, which a terminal does not have -- so the global cursor
 * moves and the ones inside do not.
 */
SCORE_ADDON_NETWORK_EXPORT void
bindTransportBroadcast(QObject& owner, Session& session, const score::DocumentContext&);

//! The other end: apply what the host reports.
SCORE_ADDON_NETWORK_EXPORT void
bindTransportMirror(QObject& owner, Session& session, const score::DocumentContext&);
}
