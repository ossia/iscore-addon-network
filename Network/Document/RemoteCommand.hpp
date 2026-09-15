#pragma once
#include <score_addon_network_export.h>

class QByteArray;
namespace score
{
struct DocumentContext;
}

namespace Network
{
/**
 * @brief Apply a command that arrived from another peer.
 *
 * Peers do not necessarily run the same build: protocols and processes are
 * registered conditionally inside plug-ins that ship everywhere, so a peer can
 * legitimately send a command this build cannot read or cannot instantiate.
 * That must not take the process down, which is what happened before -- the
 * handlers called instantiateUndoCommand() with no try/catch, so an unknown
 * command aborted (debug) or threw out of a Qt signal handler (release), on the
 * master as well as on the clients.
 *
 * Returns false when the command could not be applied, having marked the
 * document as diverged. No further remote command is applied afterwards: our
 * model no longer matches the session, and continuing would resolve paths
 * against the wrong objects.
 */
enum class OnCommandFailure
{
  //! We were expected to follow the sender, so failing to apply means our copy
  //! is now behind: mark it diverged. This is what a client does with a command
  //! relayed by the master.
  MarkDiverged,

  //! Failing to apply leaves us in a state we can still defend -- the master
  //! simply declines the client's command and does not relay it -- so nothing
  //! is marked. The sender is told instead.
  Decline
};

SCORE_ADDON_NETWORK_EXPORT bool applyRemoteCommand(
    const score::DocumentContext& ctx, const QByteArray& data,
    OnCommandFailure onFailure = OnCommandFailure::MarkDiverged);

/**
 * @brief Guard for the remote undo / redo / index handlers.
 *
 * Returns true while it is still safe to replay session-wide command-stack
 * movements onto this document.
 */
SCORE_ADDON_NETWORK_EXPORT bool
canApplyRemoteEdit(const score::DocumentContext& ctx);
}
