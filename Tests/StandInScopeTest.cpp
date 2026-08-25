// Whose stand-ins a document is allowed to fill in.
//
// A command carries what a factory would be given, not what the object would
// write, so a peer that cannot build something registers it and asks the peer
// that issued the command. That register is one per *process* while documents
// are many: draining it wholesale meant one document asking its own peer about
// another document's processes -- and answering into them with the wrong
// context, which outlives the document it came from.

#include <Process/OpaqueProcess.hpp>
#include <Process/RemoteState.hpp>

#include <Scenario/Document/Interval/IntervalModel.hpp>

#include <score/document/DocumentInterface.hpp>

#include <core/document/Document.hpp>

#include <score_test/App.hpp>
#include <score_test/Document.hpp>

#include <catch2/catch_all.hpp>

#include "SessionFixture.hpp"

using namespace SessionTest;

TEST_CASE("A document only fills in its own stand-ins", "[session][standin]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port, Network::PeerRole::Terminal);
    REQUIRE(client);

    // Something belonging to the *other* document, waiting to be filled in.
    auto& otherItv = rootInterval(*master.document);
    auto* foreign = new Process::OpaqueProcessModel{
        UuidKey<Process::ProcessModel>{}, TimeVal::zero(),
        Id<Process::ProcessModel>{9001}, &otherItv};
    otherItv.processes.add(foreign);

    auto& pending = Process::awaitingRemoteState();
    pending.clear();
    pending.push_back(foreign);
    REQUIRE(pending.size() == 1);

    // The client drains on every replicated command. Give it one.
    client->context().document.commandStack().redoAndPush(
        new Scenario::Command::ChangeElementLabel<Scenario::IntervalModel>{
            rootInterval(*client), QStringLiteral("edited")});

    REQUIRE(spin_until([&] {
      return rootInterval(*master.document).metadata().getLabel() == "edited";
    }));

    // Still there: it is the other document's, and nobody else may speak for
    // it. Draining it here would have asked this document's peer about a
    // process it has never heard of, and applied the answer with a context
    // belonging to a document that may close first.
    REQUIRE(pending.size() == 1);
    CHECK(pending.front() == foreign);

    pending.clear();
  });
}
