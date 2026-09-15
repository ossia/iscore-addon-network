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
 * Every interval, not only the root: the ones inside are what shows which part
 * of the score is running, and each gets its position from its own executor
 * component, which a terminal has none of. Sent by path, so an interval the
 * terminal does not have -- one inside a process it cannot build -- is skipped
 * rather than mistaken for another.
 */
SCORE_ADDON_NETWORK_EXPORT void
bindTransportBroadcast(QObject& owner, Session& session, const score::DocumentContext&);

//! The other end: apply what the host reports.
SCORE_ADDON_NETWORK_EXPORT void
bindTransportMirror(QObject& owner, Session& session, const score::DocumentContext&);
}
