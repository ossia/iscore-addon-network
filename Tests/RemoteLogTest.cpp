// What the host printed, on a machine that cannot see its console.
//
// The score runs on the host, so everything that complains about it -- a shader
// that would not compile, a device that would not open, a file that is not
// where the document says -- is printed over there. A terminal driving that
// score saw none of it.

#include <score/document/DocumentContext.hpp>

#include <core/document/Document.hpp>

#include <score_test/App.hpp>
#include <score_test/Document.hpp>

#include <QDebug>

#include <catch2/catch_all.hpp>

#include "SessionFixture.hpp"

using namespace SessionTest;

TEST_CASE("What the host prints reaches the peer", "[session][log]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port, Network::PeerRole::Terminal);
    REQUIRE(client);

    auto* plug = client->context().findPlugin<Network::NetworkDocumentPlugin>();
    REQUIRE(plug);

    QStringList received;
    plug->onHostLog = [&](const QStringList& lines) { received += lines; };

    qWarning() << "MARKER-FROM-HOST";

    // Batched on the UI timer, so it is not there the instant it is printed.
    REQUIRE(spin_until([&] {
      return received.join(QChar{'\n'}).contains("MARKER-FROM-HOST");
    }));
  });
}

TEST_CASE("A burst of logging does not take the host down", "[session][log]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port, Network::PeerRole::Terminal);
    REQUIRE(client);

    auto* plug = client->context().findPlugin<Network::NetworkDocumentPlugin>();
    REQUIRE(plug);

    int batches{};
    QStringList received;
    plug->onHostLog = [&](const QStringList& lines) {
      batches++;
      received += lines;
    };

    // A handler that logs while handling a log is a stack overflow, and the
    // machine that must not fall over is the one running the score.
    for(int i = 0; i < 200; i++)
      qDebug() << "burst" << i;

    REQUIRE(spin_until([&] { return received.size() >= 200; }));

    // Batched rather than one message per line: a log can burst, and the
    // socket has edits to carry.
    INFO("batches: " << batches << " for " << received.size() << " lines");
    CHECK(batches < received.size());
  });
}
