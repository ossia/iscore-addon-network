// A script typed on a terminal runs where the score runs.
//
// Document edits happen to replicate, because they are commands, so a console
// on a terminal looks like it works. Everything else runs against a document
// with no devices, no execution and no hardware behind it: Score.device("x") is
// null there and always will be. So the script itself is sent over.

#include <score/application/ScriptEvaluator.hpp>
#include <score/document/DocumentContext.hpp>

#include <core/document/Document.hpp>

#include <score_test/App.hpp>
#include <score_test/Document.hpp>

#include <catch2/catch_all.hpp>

#include "SessionFixture.hpp"

using namespace SessionTest;

namespace
{
//! Stands in for the JS plug-in: says where it ran and what it was asked.
struct RecordingEvaluator final : score::ScriptEvaluator
{
  QStringList seen;
  std::vector<const score::DocumentContext*> against;
  QString answer{"42"};

  QString evaluate(const score::DocumentContext& ctx, const QString& code) override
  {
    seen.push_back(code);
    against.push_back(&ctx);
    return answer;
  }
};

struct EvaluatorGuard
{
  score::ScriptEvaluator* previous{score::scriptEvaluator()};
  explicit EvaluatorGuard(score::ScriptEvaluator* e) { score::scriptEvaluator() = e; }
  ~EvaluatorGuard() { score::scriptEvaluator() = previous; }
};
}

TEST_CASE("A terminal's script runs on the host", "[session][script]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port, Network::PeerRole::Terminal);
    REQUIRE(client);

    RecordingEvaluator evaluator;
    EvaluatorGuard guard{&evaluator};

    // What the console does with a line typed on a terminal.
    const auto& sink = client->scriptSink();
    REQUIRE(sink);

    QString reply;
    sink("Score.device(\"mouse\")", [&](QString r) { reply = r; });

    REQUIRE(spin_until([&] { return !reply.isEmpty(); }));
    CHECK(reply == "42");

    // Ran once, with what was typed -- not paraphrased.
    REQUIRE(evaluator.seen.size() == 1);
    CHECK(evaluator.seen.front() == "Score.device(\"mouse\")");

    // And ran against the *host's* document. Both peers share one process here,
    // so the evaluator is reached either way: without this, an implementation
    // that ran the script on the terminal -- the bug this exists to prevent --
    // would record the same line and pass.
    REQUIRE(evaluator.against.size() == 1);
    CHECK(evaluator.against.front() == &master.document->context());
    CHECK(evaluator.against.front() != &client->context());
  });
}

TEST_CASE("A document that runs its own score keeps its console", "[session][script]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto* doc = score::test::new_document(ctx);
    REQUIRE(doc);

    // No session: the machine typing is the machine running, so there is
    // nowhere to send it and the console evaluates as it always has.
    CHECK(!doc->scriptSink());
  });
}

TEST_CASE("A machine that cannot run scripts says so", "[session][script]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port, Network::PeerRole::Terminal);
    REQUIRE(client);

    // A build without the JS plug-in registers no evaluator. Answering with
    // silence would look like a script that ran and printed nothing.
    EvaluatorGuard guard{nullptr};

    QString reply;
    client->scriptSink()("1 + 1", [&](QString r) { reply = r; });

    REQUIRE(spin_until([&] { return !reply.isEmpty(); }));
    CHECK(reply.contains("cannot run scripts"));
  });
}
