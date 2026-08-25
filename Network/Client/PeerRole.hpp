#pragma once
#include <cstdint>

namespace Network
{
/**
 * @brief What a peer is here to do.
 *
 * A session has always meant every peer running the score on its own machine,
 * in step with the others. That is still what a Performer is.
 *
 * A Terminal is the other thing a session can be for: editing and watching a
 * score that runs somewhere else. It mirrors the document and replicates
 * commands like any peer, but it never executes, so it opens no ports, claims
 * no MIDI or camera, and renders nothing -- which is the point when the score
 * runs on a headless box, or when the peer is a browser tab with none of that
 * to offer.
 *
 * Requested when joining and confirmed by the host, so that both ends agree
 * before the document is read: what a terminal must not do, it must not do
 * while loading either.
 */
enum class PeerRole : int32_t
{
  Performer,
  Terminal
};
}
