#pragma once
#include <score/model/Identifier.hpp>

#include <score_addon_network_export.h>

namespace score
{
struct DocumentContext;
}

namespace Process
{
class ProcessModel;
}

namespace Network
{
class Client;
class RpcChannel;

/**
 * @brief Asking a peer what an object it made actually contains.
 *
 * A command that creates something carries what a factory would be *given*, not
 * what the object would *write*. So a peer without that factory can keep the
 * document in step -- it makes a stand-in -- but the stand-in is empty, and its
 * emptiness is not a fact about the object, only about this build.
 *
 * The peer that could make it has the object. This asks for it, as JSON, and
 * feeds it through the same path that loads a stand-in from a document: ports
 * are rebuilt, controls come back, and the placeholder stops being one.
 */
SCORE_ADDON_NETWORK_EXPORT void
bindObjectQueries(RpcChannel& rpc, const score::DocumentContext& ctx);

//! Ask `peer` for the state of every stand-in created since the last call.
//! Drains the list either way, so one nobody can describe is not asked about
//! again on every edit.
//! Replace a process with the peer's version of it.
//!
//! Exposed so that what it refuses can be tested: the id comes out of the
//! answer, and the rack is re-pointed using the id we asked about.
SCORE_ADDON_NETWORK_EXPORT void applyRemoteProcessState(
    Process::ProcessModel& proc, const rapidjson::Value& state,
    const score::DocumentContext& ctx);

SCORE_ADDON_NETWORK_EXPORT void fillStandIns(
    RpcChannel& rpc, const score::DocumentContext& ctx, const Id<Client>& peer);
}
