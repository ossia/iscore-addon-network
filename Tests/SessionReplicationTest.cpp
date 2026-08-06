// A real session between two documents, over a real socket.
//
// Replication is per-document, not per-application: the edition policies bind a
// Session to a DocumentContext. So a master and a client can be two documents in
// one process, which exercises the sockets, the serialization and both policies
// without the cost of driving two applications. What it deliberately cannot
// cover is peers built differently, so the cases that matter there are provoked
// by hand rather than by having a second build.

#include <Scenario/Commands/Metadata/ChangeElementLabel.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/BaseScenario/BaseScenario.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>

#include <score/application/ApplicationContext.hpp>
#include <score/command/CommandData.hpp>
#include <score/document/DocumentContext.hpp>
#include <score/model/Identifier.hpp>
#include <score/tools/IdentifierGeneration.hpp>

#include <core/command/CommandStack.hpp>
#include <core/document/Document.hpp>
#include <core/document/DocumentModel.hpp>
#include <core/presenter/DocumentManager.hpp>

#include <QElapsedTimer>

#include <memory>
#include <stdexcept>

#include <Network/Client/LocalClient.hpp>
#include <Network/Communication/Capabilities.hpp>
#include <Network/Communication/Rpc.hpp>
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <Network/Document/MasterPolicy.hpp>
#include <Network/Session/ClientSessionBuilder.hpp>
#include <Network/Session/MasterSession.hpp>

#include <score_test/App.hpp>
#include <score_test/Document.hpp>

#include <catch2/catch_all.hpp>

namespace
{
template <typename Pred>
bool spin_until(Pred pred, int timeoutMs = 5000)
{
  QElapsedTimer t;
  t.start();
  while(!pred())
  {
    if(t.elapsed() > timeoutMs)
      return false;
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
  }
  return true;
}

Scenario::IntervalModel& rootInterval(score::Document& doc)
{
  auto& model = doc.model().modelDelegate();
  return safe_cast<Scenario::ScenarioDocumentModel&>(model).baseScenario().interval();
}

struct Master
{
  score::Document* document{};
  Network::MasterSession* session{};
  Network::NetworkDocumentPlugin* plugin{};
  int port{};
};

//! Set up a document as the host of a session, on a port the OS picks.
Master hostSession(const score::GUIApplicationContext& ctx)
{
  Master m;
  m.document = score::test::new_document(ctx);
  SCORE_ASSERT(m.document);

  auto& doc = m.document->context();
  auto* local = new Network::LocalClient(0, Id<Network::Client>(0));
  local->setName("Master");

  m.session = new Network::MasterSession(
      doc, local, Id<Network::Session>{score::random_id_generator::getRandomId()});
  m.plugin = new Network::NetworkDocumentPlugin{
      doc, new Network::MasterEditionPolicy{m.session, doc}, m.document};
  m.document->model().addPluginModel(m.plugin);
  m.port = local->localPort();

  // Joining loads the received document, and loading closes the current one if
  // it is still virgin -- which a document created a moment ago is. In a real
  // session the two are different processes and never meet; here they share a
  // DocumentManager, so give the host something to have done.
  m.document->context().document.commandStack().redoAndPush(
      new Scenario::Command::ChangeElementLabel<Scenario::IntervalModel>{
          rootInterval(*m.document), QStringLiteral("host")});
  return m;
}

Network::Capabilities* g_lastMasterCapabilities{};

//! Join a session as a second document in this same process.
//!
//! Polls rather than connecting to the builder's signals: verdigris signals do
//! not resolve by member-pointer across a shared-library boundary, since the
//! metaobject's IndexOfMethod handler is not exported.
score::Document* joinSession(const score::GUIApplicationContext& ctx, int port)
{
  auto builder = std::make_unique<Network::ClientSessionBuilder>(ctx, "127.0.0.1", port);

  if(!spin_until([&] { return builder->builtSession() != nullptr; }))
    return nullptr;

  static Network::Capabilities caps;
  caps = builder->masterCapabilities();
  g_lastMasterCapabilities = &caps;

  // The builder loads the received document as a new one, so it is the current.
  return ctx.docManager.currentDocument();
}
}

TEST_CASE("A client joins a session and receives the document", "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    REQUIRE(master.port > 0);

    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);
    CHECK(client != master.document);

    // The joining document is a real session member, not just a copy.
    auto* plug = client->context().findPlugin<Network::NetworkDocumentPlugin>();
    REQUIRE(plug);
    CHECK_FALSE(plug->diverged());
  });
}

TEST_CASE("An edit on the master reaches the client", "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    auto& masterItv = rootInterval(*master.document);
    auto& clientItv = rootInterval(*client);
    const auto label = QStringLiteral("edited on the host");

    master.document->context().document.commandStack().redoAndPush(
        new Scenario::Command::ChangeElementLabel<Scenario::IntervalModel>{
            masterItv, label});

    REQUIRE(spin_until([&] { return clientItv.metadata().getLabel() == label; }));
    CHECK(masterItv.metadata().getLabel() == label);
  });
}

TEST_CASE("A command the client cannot read stops it rather than corrupting it",
          "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    auto* plug = client->context().findPlugin<Network::NetworkDocumentPlugin>();
    REQUIRE(plug);
    REQUIRE_FALSE(plug->diverged());

    // What a peer with a plug-in we do not have sends: a command whose key means
    // nothing here. Before, this aborted in debug and threw out of a Qt signal
    // handler in release, on both ends.
    score::CommandData unknown;
    unknown.parentKey = CommandGroupKey{"NoSuchCommandGroup"};
    unknown.commandKey = CommandKey{"NoSuchCommand"};

    auto& mapi = Network::MessagesAPI::instance();
    master.session->broadcastToAllClients(
        master.session->makeMessage(mapi.command_new, unknown));

    REQUIRE(spin_until([&] { return plug->diverged(); }));
    CHECK_FALSE(plug->divergenceReason().isEmpty());

    // And it stops following: applying the rest of the stream onto a document
    // that no longer matches is what turns a gap into silent corruption.
    auto& masterItv = rootInterval(*master.document);
    auto& clientItv = rootInterval(*client);
    const auto before = clientItv.metadata().getLabel();

    master.document->context().document.commandStack().redoAndPush(
        new Scenario::Command::ChangeElementLabel<Scenario::IntervalModel>{
            masterItv, QStringLiteral("after the divergence")});

    spin_until([&] { return clientItv.metadata().getLabel() != before; }, 1000);
    CHECK(clientItv.metadata().getLabel() == before);
  });
}

TEST_CASE("Peers tell each other what they can build", "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    // The host said what it can construct. Both ends are this same build here,
    // so what it reported has to match what we see locally -- which is the
    // property that makes a difference meaningful when the builds do differ.
    REQUIRE(g_lastMasterCapabilities);
    const auto local = Network::Capabilities::local(ctx);
    CHECK_FALSE(local.protocols.isEmpty());
    CHECK_FALSE(local.processes.isEmpty());
    CHECK_FALSE(local.commands.isEmpty());

    CHECK(g_lastMasterCapabilities->protocols == local.protocols);
    CHECK(g_lastMasterCapabilities->processes == local.processes);
    CHECK(g_lastMasterCapabilities->commands == local.commands);

    CHECK(local.lacking(*g_lastMasterCapabilities).isEmpty());

    // And it is kept where the rest of score can ask, not only logged.
    auto* plug = client->context().findPlugin<Network::NetworkDocumentPlugin>();
    REQUIRE(plug);
    CHECK(plug->remoteCapabilities().protocols == local.protocols);
  });
}

TEST_CASE("A build that lacks something is told which things", "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto rich = Network::Capabilities::local(ctx);
    REQUIRE(rich.protocols.size() > 1);

    // Stand in for a peer compiled without one of the protocols -- which is
    // what Syphon on Windows or a VST in the browser actually is.
    auto poor = rich;
    const auto dropped = poor.protocols.takeFirst();

    const auto missing = poor.lacking(rich);
    CHECK(missing.protocols == QByteArrayList{dropped});
    CHECK(missing.processes.isEmpty());
    CHECK_FALSE(missing.isEmpty());
    CHECK(missing.summary().contains("1 protocols"));

    // The other direction has nothing to report: the richer build lacks nothing.
    CHECK(rich.lacking(poor).isEmpty());
    CHECK(rich.lacking(poor).summary().isEmpty());
  });
}

TEST_CASE("A peer can be asked something and answer", "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    auto* hostRpc = master.plugin->rpc();
    auto* clientPlug = client->context().findPlugin<Network::NetworkDocumentPlugin>();
    REQUIRE(hostRpc);
    REQUIRE(clientPlug);
    auto* clientRpc = clientPlug->rpc();
    REQUIRE(clientRpc);

    hostRpc->bind("test.echo", [](const rapidjson::Value& params) -> QByteArray {
      REQUIRE(params.IsObject());
      return QByteArrayLiteral(R"({"said":")")
             + QByteArray{params["say"].GetString()} + QByteArrayLiteral(R"("})");
    });

    QString answer;
    QString failure;
    clientRpc->call(
        master.session->localClient().id(), "test.echo",
        QByteArrayLiteral(R"({"say":"hello"})"),
        [&](const rapidjson::Value& result) {
      answer = QString::fromUtf8(result["said"].GetString());
        },
        [&](const QString& e) { failure = e; });

    REQUIRE(spin_until([&] { return !answer.isEmpty() || !failure.isEmpty(); }));
    CHECK(failure.isEmpty());
    CHECK(answer == "hello");
  });
}

TEST_CASE("Asking for something a peer does not offer is answered, not dropped",
          "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    // Peers do not all offer the same methods, for the same reason they do not
    // all have the same plug-ins. A caller has to hear about that rather than
    // wait forever.
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    auto* clientRpc
        = client->context().findPlugin<Network::NetworkDocumentPlugin>()->rpc();
    REQUIRE(clientRpc);

    QString failure;
    bool answered = false;
    clientRpc->call(
        master.session->localClient().id(), "test.nothingImplementsThis", {},
        [&](const rapidjson::Value&) { answered = true; },
        [&](const QString& e) { failure = e; });

    REQUIRE(spin_until([&] { return answered || !failure.isEmpty(); }));
    CHECK_FALSE(answered);
    CHECK(failure.contains("no such method"));
  });
}

TEST_CASE("A handler that fails reports back instead of taking the peer down",
          "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    master.plugin->rpc()->bind("test.fails", [](const rapidjson::Value&) -> QByteArray {
      throw std::runtime_error{"the camera is unplugged"};
    });

    auto* clientRpc
        = client->context().findPlugin<Network::NetworkDocumentPlugin>()->rpc();
    QString failure;
    clientRpc->call(
        master.session->localClient().id(), "test.fails", {},
        [](const rapidjson::Value&) {}, [&](const QString& e) { failure = e; });

    REQUIRE(spin_until([&] { return !failure.isEmpty(); }));
    CHECK(failure == "the camera is unplugged");
  });
}
