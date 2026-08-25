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
 * @brief What the host is saying, on a machine that cannot see it.
 *
 * The score runs on the host: the decoders, the devices and the executor are
 * all there, and so is everything they complain about. A terminal driving that
 * score sees none of it -- a shader that failed to compile, a device that could
 * not open, a file that is not where the document says -- because those
 * messages are printed on the other machine's console.
 *
 * Batched on the UI timer: a log can burst, and one message per line would put
 * the socket to work saying nothing.
 */
SCORE_ADDON_NETWORK_EXPORT void
bindLogBroadcast(QObject& owner, Session& session, const score::DocumentContext&);

//! The other end: shown in this machine's message log, marked as the host's so
//! it is not mistaken for something that happened here.
SCORE_ADDON_NETWORK_EXPORT void
bindLogDisplay(QObject& owner, Session& session, const score::DocumentContext&);
}
