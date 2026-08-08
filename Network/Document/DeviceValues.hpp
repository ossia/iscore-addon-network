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
class ClientSession;

/**
 * @brief Editing a value in the device tree, from a machine that has no devices.
 *
 * Setting a value has always meant "send it to the device", and the device is
 * on the machine running the score. So the edit travels and is performed there,
 * by the same code an ordinary document uses -- the terminal is not a second
 * OSC client, it asks the one that already exists.
 */
SCORE_ADDON_NETWORK_EXPORT void
bindValueForwarding(ClientSession& session, const score::DocumentContext&);

//! The other end: perform what a peer asked for, on this machine's devices.
SCORE_ADDON_NETWORK_EXPORT void
bindValueSetter(QObject& owner, Session& session, const score::DocumentContext&);

/**
 * @brief Telling peers what this machine's devices are reporting.
 *
 * The opposite direction, and not the same thing: above, a peer asks for a
 * value and the host performs it. Here the host has *seen* one -- a mouse
 * moved, a fader was touched -- and a peer with no devices has no other way to
 * know. Coalesced per address, since a mouse reports far more often than
 * anybody can read: what a peer needs is the current value, not every one it
 * passed through.
 */
SCORE_ADDON_NETWORK_EXPORT void
bindValueBroadcast(QObject& owner, Session& session, const score::DocumentContext&);

//! The receiving end: show it. Not "set it" -- performing a notification would
//! send it back to the machine it came from.
SCORE_ADDON_NETWORK_EXPORT void
bindValueDisplay(QObject& owner, Session& session, const score::DocumentContext&);
}
