// What a terminal *shows* of a process it has just added.
//
// A terminal cannot trust the creation data a command carries -- it described
// another machine -- so it asks the peer what the process really is and applies
// the answer. That replaces the process, and the replacement has to end up
// drawn: a process that appears for a frame and then vanishes is worse than one
// that was never added. Needs the GUI stack, since none of this is reachable
// without layers, and a binary cannot mix the two application fixtures.

#include "SessionFixture.hpp"

#include <Process/ProcessList.hpp>
#include <Process/RemoteState.hpp>

#include <Scenario/Commands/CommandAPI.hpp>
#include <Scenario/Commands/Interval/AddProcessToInterval.hpp>
#include <Scenario/Document/Interval/FullView/FullViewIntervalPresenter.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentPresenter.hpp>

#include <score/document/DocumentContext.hpp>
#include <score/selection/SelectionStack.hpp>

#include <core/document/DocumentPresenter.hpp>

#include <score_test/App.hpp>

#include <catch2/catch_test_macros.hpp>

using namespace SessionTest;

namespace
{
const UuidKey<Process::ProcessModel> automationKey{
    score::uuids::string_generator::compute("d2a67bd8-5d3f-404e-b6e9-e350cf2a833f")};

std::size_t displayedSlots(const score::GUIApplicationContext& ctx, score::Document& doc)
{
  ctx.docManager.setCurrentDocument(ctx, &doc);
  QApplication::processEvents();

  auto* pres = safe_cast<Scenario::ScenarioDocumentPresenter*>(
      doc.presenter()->presenterDelegate());
  SCORE_ASSERT(pres);
  pres->setDisplayedInterval(&rootInterval(doc));
  QApplication::processEvents();

  auto* full = dynamic_cast<Scenario::FullViewIntervalPresenter*>(
      pres->displayedIntervalPresenter());
  SCORE_ASSERT(full);
  return full->getSlots().size();
}
}

TEST_CASE("A process added on a terminal stays drawn", "[session]")
{
  qputenv("SCORE_DISABLE_LIBRARY", "1");

  score::test::run_in_gui_app([](const score::GUIApplicationContext& ctx) {
    REQUIRE(ctx.interfaces<Process::ProcessFactoryList>().get(automationKey));

    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port, Network::PeerRole::Terminal);
    REQUIRE(client);

    const auto before = displayedSlots(ctx, *client);
    auto& clientItv = rootInterval(*client);

    // The gesture the library panel performs: a process and a slot to show it
    // in, as one command.
    {
      Scenario::Command::Macro m{
          new Scenario::Command::AddProcessInNewBoxMacro, client->context()};
      REQUIRE(m.createProcessInNewSlot(clientItv, automationKey, {}));
      m.commit();
    }
    REQUIRE(spin_until([&] { return clientItv.processes.size() == 2; }));
    CHECK(displayedSlots(ctx, *client) == before + 1);
    auto& madeHereRef = *clientItv.processes.begin();
    const auto* madeHere = &madeHereRef;

    // A drop leaves what it created selected, and the inspector then holds it.
    // Replacing a process nothing is looking at is not the same case.
    client->context().selectionStack.pushNewSelection({&madeHereRef});
    QApplication::processEvents();

    // Then the answer to "what is this process really" arrives and is applied.
    spin_until([] { return false; }, 2000);

    REQUIRE(madeHere != &*clientItv.processes.begin());

    // Still one process, and still somewhere to see it.
    CHECK(clientItv.processes.size() == 2);
    CHECK(displayedSlots(ctx, *client) == before + 1);
  });
}
