#pragma once
#include <score_addon_network_export.h>

#include <cstddef>

namespace Network
{
class Group;
class Session;

/**
 * @brief How many members of a group will actually answer a consensus.
 *
 * A trigger shared between machines waits for the group to agree, and how many
 * peers that means is what decides when "all of them" has happened. A terminal
 * is in the session but never executes, so it never has an opinion about a
 * trigger: counting it means waiting for a vote that cannot arrive, and the
 * whole session stops on that trigger for good.
 *
 * Ids in the group with no client behind them are still counted, which is the
 * pre-existing question of what a group means once a peer has disconnected --
 * a different problem, and not one to answer silently here.
 */
SCORE_ADDON_NETWORK_EXPORT std::size_t
executingClients(const Session& session, const Group& group) noexcept;
}
