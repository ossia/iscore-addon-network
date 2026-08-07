// A real session between two documents, over a real socket.
//
// Replication is per-document, not per-application: the edition policies bind a
// Session to a DocumentContext. So a master and a client can be two documents in
// one process, which exercises the sockets, the serialization and both policies
// without the cost of driving two applications. What it deliberately cannot
// cover is peers built differently, so the cases that matter there are provoked
// by hand rather than by having a second build.

#include <Scenario/Commands/Metadata/ChangeElementLabel.hpp>
#include <Scenario/Commands/Interval/AddOnlyProcessToInterval.hpp>
#include <Process/Process.hpp>
#include <Process/ProcessList.hpp>
#include <Process/OpaqueProcess.hpp>
#include <score/model/EntitySerialization.hpp>
#include <score/plugins/SerializableHelpers.hpp>
#include <score/document/DocumentInterface.hpp>
#include <ossia/detail/algorithms.hpp>
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
#include <QFile>
#include <QTemporaryDir>

#include <algorithm>

#include <memory>
#include <stdexcept>

#include <Network/Client/LocalClient.hpp>
#include <Network/Communication/Capabilities.hpp>
#include <Network/Communication/MessageMapper.hpp>
#include <score/serialization/JSONVisitor.hpp>
#include <score/tools/Environment.hpp>
#include <score/tools/FilePath.hpp>

#include <Device/Protocol/ProtocolFactoryInterface.hpp>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <Explorer/Explorer/DeviceExplorerModel.hpp>
#include <Network/Client/RemoteClient.hpp>
#include <Device/Protocol/DeviceCatalog.hpp>
#include <Network/Group/Group.hpp>
#include <Network/Group/GroupExecution.hpp>
#include <Network/Group/GroupManager.hpp>
#include <Device/Protocol/ProtocolList.hpp>

#include <Network/Communication/Rpc.hpp>
#include <Network/Document/RemoteEnvironment.hpp>
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <Network/Document/MasterPolicy.hpp>
#include <Network/Session/ClientSessionBuilder.hpp>
#include <Network/Session/MasterSession.hpp>

#include <score_test/App.hpp>
#include <score_test/ProbeProtocol.hpp>
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
score::Document* joinSession(
    const score::GUIApplicationContext& ctx, int port,
    Network::PeerRole role = Network::PeerRole::Performer)
{
  auto builder
      = std::make_unique<Network::ClientSessionBuilder>(ctx, "127.0.0.1", port, role);

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

TEST_CASE("A client can ask the host which protocols it has", "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    // Capability exchange carries the uuids, but nothing a person can read. To
    // offer the host's protocols in a list, the names have to come across too.
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    auto* clientRpc
        = client->context().findPlugin<Network::NetworkDocumentPlugin>()->rpc();
    REQUIRE(clientRpc);

    int count = -1;
    bool named = false;
    QString failure;
    clientRpc->call(
        master.session->localClient().id(), "device.protocols", {},
        [&](const rapidjson::Value& result) {
      REQUIRE(result.IsArray());
      count = result.Size();
      for(const auto& p : result.GetArray())
      {
        if(p.HasMember("uuid") && p.HasMember("name") && p.HasMember("category")
           && p["name"].GetStringLength() > 0)
          named = true;
      }
        },
        [&](const QString& e) { failure = e; });

    REQUIRE(spin_until([&] { return count >= 0 || !failure.isEmpty(); }));
    CHECK(failure.isEmpty());
    CHECK(count > 0);
    CHECK(named);
  });
}

TEST_CASE("A client can ask the host what is plugged into it", "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    auto* clientRpc
        = client->context().findPlugin<Network::NetworkDocumentPlugin>()->rpc();

    // Ask about every protocol the host reported. Most have nothing to
    // enumerate -- an OSC device is not plugged in anywhere -- so what is
    // asserted is that each is answered with a well-formed list, and that at
    // least one of them really did find hardware, since a path that always
    // returned nothing would pass a shape check just as well.
    int answers = 0;
    int found = 0;
    QString failure;
    QByteArrayList uuids;
    for(auto& p : ctx.interfaces<Device::ProtocolFactoryList>())
      uuids.push_back(score::uuids::toByteArray(p.concreteKey().impl()));

    for(const auto& uuid : uuids)
    {
      clientRpc->call(
          master.session->localClient().id(), "device.enumerate",
          QByteArrayLiteral(R"({"protocol":")") + uuid + QByteArrayLiteral(R"("})"),
          [&](const rapidjson::Value& result) {
        REQUIRE(result.IsArray());
        for(const auto& d : result.GetArray())
        {
          CHECK(d.HasMember("category"));
          CHECK(d.HasMember("name"));
          CHECK(d.HasMember("settings"));
          found++;
        }
        answers++;
          },
          [&](const QString& e) { failure = e; });
    }

    REQUIRE(spin_until(
        [&] { return answers == uuids.size() || !failure.isEmpty(); }, 20000));
    CHECK(failure.isEmpty());
    CHECK(answers == uuids.size());
    CHECK(found > 0);
  });
}

TEST_CASE("Asking about a protocol the host does not have says so", "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    auto* clientRpc
        = client->context().findPlugin<Network::NetworkDocumentPlugin>()->rpc();

    QString failure;
    bool answered = false;
    clientRpc->call(
        master.session->localClient().id(), "device.enumerate",
        QByteArrayLiteral(R"({"protocol":"11111111-2222-3333-4444-555555555555"})"),
        [&](const rapidjson::Value&) { answered = true; },
        [&](const QString& e) { failure = e; });

    REQUIRE(spin_until([&] { return answered || !failure.isEmpty(); }));
    CHECK_FALSE(answered);
    CHECK(failure.contains("no protocol"));
  });
}

namespace
{
//! Both implementations, driven through the same interface, so that what is
//! asserted is the contract rather than either one's internals.
void checkEnvironment(score::Environment& env, const QString& projectDir)
{
  const score::Uri file{score::UriScheme::Project, "notes.txt"};
  const QByteArray body = "written through the environment";

  QString failure;
  bool written = false;
  env.write(file, body, [&] { written = true; }, [&](const QString& e) { failure = e; });
  REQUIRE(spin_until([&] { return written || !failure.isEmpty(); }));
  INFO(failure.toStdString());
  REQUIRE(written);

  // It really is a file on the host, wherever the caller was.
  CHECK(QFile::exists(projectDir + "/notes.txt"));

  QByteArray read;
  env.read(file, [&](QByteArray d) { read = d; }, [&](const QString& e) { failure = e; });
  REQUIRE(spin_until([&] { return !read.isEmpty() || !failure.isEmpty(); }));
  CHECK(failure.isEmpty());
  CHECK(read == body);

  std::vector<score::DirEntry> listed;
  bool didList = false;
  env.list(
      score::Uri{score::UriScheme::Project, {}},
      [&](std::vector<score::DirEntry> e) {
    listed = std::move(e);
    didList = true;
      },
      [&](const QString& e) { failure = e; });
  REQUIRE(spin_until([&] { return didList || !failure.isEmpty(); }));
  CHECK(failure.isEmpty());
  CHECK(std::any_of(listed.begin(), listed.end(), [](const score::DirEntry& e) {
    return e.name == "notes.txt" && !e.directory && e.size > 0;
  }));

}


//! A saved-looking document, since <PROJECT>: is relative to where the document
//! is and resolving it canonicalises: the file has to exist for its folder to
//! have a canonical path at all.
QString giveProjectFolder(score::Document& doc, QTemporaryDir& dir)
{
  const QString documentPath = dir.path() + "/host.score";
  QFile f{documentPath};
  SCORE_ASSERT(f.open(QIODevice::WriteOnly));
  f.close();

  doc.metadata().setFileName(documentPath);
  return QFileInfo{documentPath}.canonicalPath();
}
}

TEST_CASE("The files of a score can be reached on this machine", "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto* doc = score::test::new_document(ctx);
    REQUIRE(doc);

    QTemporaryDir project;
    REQUIRE(project.isValid());
    const auto projectDir = giveProjectFolder(*doc, project);
    REQUIRE_FALSE(projectDir.isEmpty());

    score::LocalEnvironment local{doc->context()};
    CHECK(local.isLocal());
    CHECK_FALSE(local.resolve(score::Uri{score::UriScheme::Project, "a"}).isEmpty());

    checkEnvironment(local, projectDir);
  });
}

TEST_CASE("The files of a score can be reached from another machine", "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);

    QTemporaryDir project;
    REQUIRE(project.isValid());
    const auto projectDir = giveProjectFolder(*master.document, project);
    REQUIRE_FALSE(projectDir.isEmpty());

    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);
    auto* rpc = client->context().findPlugin<Network::NetworkDocumentPlugin>()->rpc();
    REQUIRE(rpc);

    Network::RemoteEnvironment remote{*rpc, master.session->localClient().id()};

    // The point of the distinction: there is no path here that leads to it, so
    // callers cannot quietly fall back to opening one.
    CHECK_FALSE(remote.isLocal());
    CHECK(remote.resolve(score::Uri{score::UriScheme::Project, "a"}).isEmpty());

    checkEnvironment(remote, projectDir);

    // Locally an absolute path is just a path the user chose. Across a session
    // it names a place on somebody else's machine, and the schemes are the
    // whole of what keeps a peer inside the score's own files.
    QString refused;
    bool got = false;
    remote.read(
        score::Uri{score::UriScheme::Absolute, "/etc/passwd"},
        [&](QByteArray) { got = true; }, [&](const QString& e) { refused = e; });
    REQUIRE(spin_until([&] { return got || !refused.isEmpty(); }));
    CHECK_FALSE(got);
    CHECK(refused.contains("project, library and cache"));
  });
}

TEST_CASE("A joined document knows its files are elsewhere", "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);

    QTemporaryDir project;
    REQUIRE(project.isValid());
    const auto projectDir = giveProjectFolder(*master.document, project);

    // Something for the host to have, so that a path resolving to nothing on
    // the client cannot be confused with a file that simply is not there.
    {
      QFile f{projectDir + "/sound.wav"};
      REQUIRE(f.open(QIODevice::WriteOnly));
      f.write("not really audio");
    }

    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    // The host resolves its own files to real paths.
    CHECK(master.document->environment().isLocal());
    const auto hostPath
        = score::locateFilePath("<PROJECT>:sound.wav", master.document->context());
    CHECK(hostPath == projectDir + "/sound.wav");

    // The client cannot place it, and hands the reference back untouched --
    // several callers write this result into the model and relativize it again
    // on save, so an empty string would erase the reference for every machine.
    CHECK_FALSE(client->environment().isLocal());
    CHECK(
        score::locateFilePath("<PROJECT>:sound.wav", client->context())
        == "<PROJECT>:sound.wav");

    // It reads them through the session instead.
    QByteArray got;
    QString failure;
    client->environment().read(
        score::Uri{score::UriScheme::Project, "sound.wav"},
        [&](QByteArray d) { got = d; }, [&](const QString& e) { failure = e; });
    REQUIRE(spin_until([&] { return !got.isEmpty() || !failure.isEmpty(); }));
    CHECK(failure.isEmpty());
    CHECK(got == "not really audio");
  });
}

TEST_CASE("A peer cannot reach outside the score's own files", "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);

    QTemporaryDir project;
    REQUIRE(project.isValid());
    const auto projectDir = giveProjectFolder(*master.document, project);

    // Something outside the project, standing in for whatever the user has
    // that a peer has no business touching.
    QTemporaryDir elsewhere;
    REQUIRE(elsewhere.isValid());
    const QString victim = elsewhere.path() + "/private";
    {
      QFile f{victim};
      REQUIRE(f.open(QIODevice::WriteOnly));
      f.write("must survive");
    }

    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);
    auto* rpc = client->context().findPlugin<Network::NetworkDocumentPlugin>()->rpc();
    REQUIRE(rpc);
    const auto host = master.session->localClient().id();

    const auto refuse = [&](const QByteArray& method, const QByteArray& params) {
      QString failure;
      bool answered = false;
      rpc->call(
          host, method, params, [&](const rapidjson::Value&) { answered = true; },
          [&](const QString& e) { failure = e; });
      REQUIRE(spin_until([&] { return answered || !failure.isEmpty(); }));
      INFO(method.toStdString() + " " + params.toStdString());
      CHECK_FALSE(answered);
      CHECK_FALSE(failure.isEmpty());
    };

    const auto quoted = [](const QString& s) {
      return QByteArrayLiteral(R"({"uri":")") + s.toUtf8() + QByteArrayLiteral(R"("})");
    };

    // A path with no scheme resolves against the project, and nothing about it
    // says it stays there.
    const QString escape
        = QStringLiteral("../") + QFileInfo{elsewhere.path()}.fileName() + "/private";
    refuse("fs.read", quoted(escape));
    refuse("fs.list", quoted(QStringLiteral("..")));
    refuse("fs.read", quoted(victim));
    refuse("fs.read", quoted(QStringLiteral("<PROJECT>:../../etc/passwd")));

    // The write side is what actually destroys something: opening for writing
    // truncates, so a refusal that came after the open would be too late.
    const auto writeParams = QByteArrayLiteral(R"({"uri":")") + escape.toUtf8()
                             + QByteArrayLiteral(R"(","data":"b3ducmQ="})");
    refuse("fs.write", writeParams);
    refuse(
        "fs.write",
        QByteArrayLiteral(R"({"uri":"<PROJECT>:../../x","data":"b3ducmQ="})"));

    // Untouched, not merely restored afterwards.
    QFile check{victim};
    REQUIRE(check.open(QIODevice::ReadOnly));
    CHECK(check.readAll() == "must survive");

    // And the legitimate case still works.
    QString failure;
    bool written = false;
    rpc->call(
        host, "fs.write",
        QByteArrayLiteral(R"({"uri":"<PROJECT>:sub/ok.txt","data":"b3ducmQ="})"),
        [&](const rapidjson::Value&) { written = true; },
        [&](const QString& e) { failure = e; });
    REQUIRE(spin_until([&] { return written || !failure.isEmpty(); }));
    INFO(failure.toStdString());
    CHECK(written);
    CHECK(QFile::exists(projectDir + "/sub/ok.txt"));
  });
}

TEST_CASE("A question to a peer that never answers still comes back", "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    auto* clientRpc
        = client->context().findPlugin<Network::NetworkDocumentPlugin>()->rpc();
    REQUIRE(clientRpc);

    // Nobody by that id, so sendMessage drops it silently. Without a timeout
    // neither callback ever runs and the caller waits for good.
    QString failure;
    bool answered = false;
    clientRpc->call(
        Id<Network::Client>{31337}, "device.protocols", {},
        [&](const rapidjson::Value&) { answered = true; },
        [&](const QString& e) { failure = e; }, 300);

    REQUIRE(spin_until([&] { return answered || !failure.isEmpty(); }));
    CHECK_FALSE(answered);
    CHECK_FALSE(failure.isEmpty());
  });
}

TEST_CASE("A diverged peer stops sending its edits too", "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    auto* plug = client->context().findPlugin<Network::NetworkDocumentPlugin>();
    REQUIRE(plug);

    score::CommandData unknown;
    unknown.parentKey = CommandGroupKey{"NoSuchCommandGroup"};
    unknown.commandKey = CommandKey{"NoSuchCommand"};
    master.session->broadcastToAllClients(
        master.session->makeMessage(
            Network::MessagesAPI::instance().command_new, unknown));
    REQUIRE(spin_until([&] { return plug->diverged(); }));

    // Its edits are now expressed against a document nobody else has, and the
    // paths in them name different objects on the other side.
    auto& masterItv = rootInterval(*master.document);
    const auto before = masterItv.metadata().getLabel();

    auto& clientItv = rootInterval(*client);
    client->context().document.commandStack().redoAndPush(
        new Scenario::Command::ChangeElementLabel<Scenario::IntervalModel>{
            clientItv, QStringLiteral("edited after diverging")});

    spin_until([&] { return masterItv.metadata().getLabel() != before; }, 1000);
    CHECK(masterItv.metadata().getLabel() == before);
  });
}

TEST_CASE("The master ignores stack movements it cannot make", "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    // undoQuiet pops whether or not there is anything to pop, and setIndexQuiet
    // walks toward whatever number arrives.
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    auto& stack = master.document->context().document.commandStack();
    const auto before = stack.size();

    auto* session = master.session;
    auto& mapi = Network::MessagesAPI::instance();
    for(int i = 0; i < 20; i++)
      session->mapper().map(session->makeMessage(mapi.command_undo));
    session->mapper().map(session->makeMessage(mapi.command_index, (int32_t)999999));
    session->mapper().map(session->makeMessage(mapi.command_index, (int32_t)-42));

    QCoreApplication::processEvents();
    CHECK(stack.size() == before);
    CHECK(stack.currentIndex() >= 0);
    CHECK(stack.currentIndex() <= stack.size());
  });
}

// ---------------------------------------------------------------------------
// Terminals: peers that edit and watch a score running somewhere else.
// ---------------------------------------------------------------------------

TEST_CASE("A terminal joins as one, and the host knows it", "[session][terminal]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port, Network::PeerRole::Terminal);
    REQUIRE(client);

    // The role the client asked for is the one its document was built with:
    // devices are instantiated while loading, so a role settled afterwards
    // would have settled too late.
    CHECK(client->role() == score::DocumentRole::Terminal);

    // And the host agrees, rather than each end holding its own opinion.
    const auto& peers = master.session->remoteClients();
    REQUIRE(peers.size() == 1);
    CHECK(peers.front()->role() == Network::PeerRole::Terminal);
  });
}

TEST_CASE("An ordinary peer is still a performer", "[session][terminal]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    CHECK(client->role() == score::DocumentRole::Local);

    const auto& peers = master.session->remoteClients();
    REQUIRE(peers.size() == 1);
    CHECK(peers.front()->role() == Network::PeerRole::Performer);
  });
}

TEST_CASE("A terminal edits the score like any peer", "[session][terminal]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port, Network::PeerRole::Terminal);
    REQUIRE(client);

    auto& masterItv = rootInterval(*master.document);
    auto& clientItv = rootInterval(*client);

    // Host -> terminal.
    const auto fromHost = QStringLiteral("from the host");
    master.document->context().document.commandStack().redoAndPush(
        new Scenario::Command::ChangeElementLabel<Scenario::IntervalModel>{
            masterItv, fromHost});
    REQUIRE(spin_until([&] { return clientItv.metadata().getLabel() == fromHost; }));

    // Terminal -> host. This is the whole point: not running the score is not
    // the same as not editing it.
    const auto fromTerminal = QStringLiteral("from the terminal");
    client->context().document.commandStack().redoAndPush(
        new Scenario::Command::ChangeElementLabel<Scenario::IntervalModel>{
            clientItv, fromTerminal});
    REQUIRE(spin_until([&] { return masterItv.metadata().getLabel() == fromTerminal; }));

    auto* plug = client->context().findPlugin<Network::NetworkDocumentPlugin>();
    REQUIRE(plug);
    CHECK_FALSE(plug->diverged());
  });
}

TEST_CASE("A terminal builds none of the host's devices", "[session][terminal]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    score::test::register_probe_protocol(ctx);

    auto master = hostSession(ctx);

    // The host has a device. It is a protocol both ends have, so a terminal
    // declining to build it is a decision and not an absence.
    auto& hostDevices
        = master.document->context().plugin<Explorer::DeviceDocumentPlugin>();
    hostDevices.explorer().addDevice(
        score::test::probe_device_node(QStringLiteral("stagebox")));

    score::test::ProbeProtocolFactory::requests = 0;
    auto* client = joinSession(ctx, master.port, Network::PeerRole::Terminal);
    REQUIRE(client);

    CHECK(score::test::ProbeProtocolFactory::requests == 0);

    // The device is still there to be edited, just not opened.
    auto& clientDevices = client->context().plugin<Explorer::DeviceDocumentPlugin>();
    REQUIRE(clientDevices.rootNode().childCount() == 1);
    CHECK(
        clientDevices.rootNode().childAt(0).get<Device::DeviceSettings>().name
        == QStringLiteral("stagebox"));
    CHECK(clientDevices.list().findDevice(QStringLiteral("stagebox")) == nullptr);
  });
}

TEST_CASE("A performer does build the host's devices", "[session][terminal]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    score::test::register_probe_protocol(ctx);

    auto master = hostSession(ctx);
    auto& hostDevices
        = master.document->context().plugin<Explorer::DeviceDocumentPlugin>();
    hostDevices.explorer().addDevice(
        score::test::probe_device_node(QStringLiteral("stagebox")));

    score::test::ProbeProtocolFactory::requests = 0;
    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    // The precondition of the case above: joining normally does open them.
    CHECK(score::test::ProbeProtocolFactory::requests == 1);
  });
}

TEST_CASE("A terminal is not waited on for a shared trigger", "[session][terminal]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port, Network::PeerRole::Terminal);
    REQUIRE(client);

    auto& gm = master.plugin->groupManager();
    auto* group = gm.group(gm.defaultGroup());
    REQUIRE(group);

    // The host is in the default group and executes, so it counts.
    REQUIRE(group->clients().size() == 1);
    CHECK(Network::executingClients(*master.session, *group) == 1);

    // Put the terminal in the group, as a person could from the group panel.
    // It is a member, but it will never have an opinion about a trigger:
    // counting it means OnAll never becomes true and the trigger never fires
    // for anybody.
    REQUIRE(master.session->remoteClients().size() == 1);
    group->addClient(master.session->remoteClients().front()->id());

    REQUIRE(group->clients().size() == 2);
    CHECK(Network::executingClients(*master.session, *group) == 1);

    // An id naming nobody is a disconnection, which is a different question:
    // it stays counted rather than being silently written off here.
    group->addClient(Id<Network::Client>{999});
    CHECK(Network::executingClients(*master.session, *group) == 2);
  });
}

TEST_CASE("A terminal follows the host's playhead", "[session][terminal]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port, Network::PeerRole::Terminal);
    REQUIRE(client);

    auto& hostItv = rootInterval(*master.document);
    auto& termItv = rootInterval(*client);

    REQUIRE(termItv.duration.playPercentage() == 0.);

    // What the host's executor does as the score runs. A terminal has no
    // executor, so without the report its cursor never moves at all.
    hostItv.duration.setPlayPercentage(0.5);

    REQUIRE(spin_until([&] { return termItv.duration.playPercentage() > 0.; }));
    CHECK(termItv.duration.playPercentage() == Catch::Approx(0.5));

    hostItv.duration.setPlayPercentage(0.75);
    REQUIRE(
        spin_until([&] { return termItv.duration.playPercentage() > 0.6; }));
    CHECK(termItv.duration.playPercentage() == Catch::Approx(0.75));
  });
}

// ---------------------------------------------------------------------------
// Commands that name factories the receiving peer does not have.
// ---------------------------------------------------------------------------

TEST_CASE("Adding a process the peer cannot make does not stop it", "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    auto* plug = client->context().findPlugin<Network::NetworkDocumentPlugin>();
    REQUIRE(plug);
    REQUIRE_FALSE(plug->diverged());

    auto& masterItv = rootInterval(*master.document);
    auto& clientItv = rootInterval(*client);

    // A process key no build registers, as a peer with a plug-in we lack would
    // send. Before, redo asserted on the missing factory: that threw out of the
    // socket callback and the client marked itself diverged, which also stops
    // it sending -- "it never executes commands again".
    const auto absent = UuidKey<Process::ProcessModel>::fromString(
        QStringLiteral("77777777-8888-9999-aaaa-bbbbbbbbbbbb"));

    auto* cmd = new Scenario::Command::AddOnlyProcessToInterval{
        masterItv, absent, QString{}, QPointF{}};
    master.document->context().document.commandStack().redoAndPush(cmd);

    // The base interval already holds its Scenario, so the new process is one
    // more than what was there -- counting to one would have been satisfied
    // before the command ever arrived.
    const auto before = clientItv.processes.size();
    REQUIRE(before >= 1);

    // The host made a stand-in of its own -- it has no such factory either --
    // and the client followed instead of stopping.
    REQUIRE(spin_until([&] { return clientItv.processes.size() == before + 1; }));
    CHECK_FALSE(plug->diverged());

    auto it = ossia::find_if(clientItv.processes, [&](const Process::ProcessModel& p) {
      return p.concreteKey() == absent;
    });
    REQUIRE(it != clientItv.processes.end());

    // A stand-in starts with no state -- the command carried what a factory
    // would be given, not what the process would write -- so it asks the peer
    // that made it, and stops being a placeholder once the answer arrives.
    auto* opaque = dynamic_cast<const Process::OpaqueProcessModel*>(&*it);
    REQUIRE(opaque);
    REQUIRE(spin_until([&] { return !opaque->incomplete(); }));

    // Editing still works afterwards, which is the part that was lost.
    const auto label = QStringLiteral("still listening");
    master.document->context().document.commandStack().redoAndPush(
        new Scenario::Command::ChangeElementLabel<Scenario::IntervalModel>{
            masterItv, label});
    REQUIRE(spin_until([&] { return clientItv.metadata().getLabel() == label; }));
  });
}

TEST_CASE("A peer can be asked what an object it made contains", "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    auto* plug = client->context().findPlugin<Network::NetworkDocumentPlugin>();
    REQUIRE(plug);
    auto* rpc = plug->rpc();
    REQUIRE(rpc);

    // The base interval's Scenario, which both ends have: what matters here is
    // that the answer is that object's serialization, addressed by path.
    auto& hostItv = rootInterval(*master.document);
    REQUIRE_FALSE(hostItv.processes.empty());
    auto& hostProc = *hostItv.processes.begin();

    JSONReader pathJson;
    pathJson.readFrom(score::IDocument::path(hostProc).unsafePath());

    rapidjson::StringBuffer buf;
    JsonWriter w{buf};
    w.StartObject();
    w.Key("path");
    {
      rapidjson::Document d;
      const auto bytes = pathJson.toByteArray();
      d.Parse(bytes.data(), bytes.size());
      d.Accept(w);
    }
    w.EndObject();

    QByteArray answer;
    bool failed = false;
    rpc->call(
        master.session->localClient().id(), "object.state",
        QByteArray{buf.GetString(), (int)buf.GetLength()},
        [&](const rapidjson::Value& result) {
      rapidjson::StringBuffer out;
      JsonWriter ow{out};
      result.Accept(ow);
      answer = QByteArray{out.GetString(), (int)out.GetLength()};
        },
        [&](const QString&) { failed = true; });

    REQUIRE(spin_until([&] { return !answer.isEmpty() || failed; }));
    REQUIRE_FALSE(failed);

    // It really is that process, not an empty object: the key it reports is the
    // one the object has.
    rapidjson::Document got;
    got.Parse(answer.data(), answer.size());
    REQUIRE_FALSE(got.HasParseError());
    REQUIRE(got.IsObject());
    REQUIRE(got.HasMember("uuid"));
  });
}

TEST_CASE("A stand-in given its state stops being a placeholder", "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto* doc = score::test::new_document(ctx);
    REQUIRE(doc);

    auto& itv = rootInterval(*doc);
    const auto absent = UuidKey<Process::ProcessModel>::fromString(
        QStringLiteral("77777777-8888-9999-aaaa-bbbbbbbbbbbb"));

    auto& facs = ctx.interfaces<Process::ProcessFactoryList>();
    auto* stand = facs.makeMissing(
        absent, TimeVal::fromMsecs(1000), Id<Process::ProcessModel>{9}, &itv);
    REQUIRE(stand);

    auto* opaque = dynamic_cast<Process::OpaqueProcessModel*>(stand);
    REQUIRE(opaque);
    REQUIRE(opaque->incomplete());
    REQUIRE(opaque->inlets().empty());

    // What the peer that could make it would have answered: its serialization,
    // ports and all.
    const QByteArray state
        = QStringLiteral(R"({"uuid":"%1","Inlets":[],"Outlets":[],)"
                         R"("PluginOwnMember":42})")
              .arg(QStringLiteral("77777777-8888-9999-aaaa-bbbbbbbbbbbb"))
              .toUtf8();
    rapidjson::Document d;
    d.Parse(state.data(), state.size());
    REQUIRE_FALSE(d.HasParseError());

    opaque->setState(d);

    CHECK_FALSE(opaque->incomplete());

    // And what it now writes out carries the plug-in's member, so a machine
    // that has the plug-in gets it back rather than an empty process.
    JSONReader r;
    r.readFrom(static_cast<Process::ProcessModel&>(*opaque));
    const auto written = r.toByteArray();
    CHECK(written.contains("PluginOwnMember"));

    delete opaque;
  });
}

TEST_CASE("A peer can be asked for its library", "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    auto* rpc = client->context().findPlugin<Network::NetworkDocumentPlugin>()->rpc();
    REQUIRE(rpc);

    QByteArray answer;
    bool failed = false;
    rpc->call(
        master.session->localClient().id(), "library.processes",
        QByteArrayLiteral("{}"),
        [&](const rapidjson::Value& result) {
      rapidjson::StringBuffer out;
      JsonWriter ow{out};
      result.Accept(ow);
      answer = QByteArray{out.GetString(), (int)out.GetLength()};
        },
        [&](const QString&) { failed = true; });

    REQUIRE(spin_until([&] { return !answer.isEmpty() || failed; }));
    REQUIRE_FALSE(failed);

    rapidjson::Document got;
    got.Parse(answer.data(), answer.size());
    REQUIRE_FALSE(got.HasParseError());
    REQUIRE(got.IsArray());
    REQUIRE(got.Size() > 0);

    // A tree, not a list. Categories nest -- "Plugins/Faust" is two levels, not
    // one name with a slash in it -- and most of a library is not a factory at
    // all: shaders, Faust programs and presets are entries built by scanning
    // files. Sending the factory list reproduces neither.
    bool nested = false;
    for(const auto& e : got.GetArray())
    {
      REQUIRE(e.IsObject());
      REQUIRE(e.HasMember("name"));
      if(e.HasMember("children") && e["children"].IsArray()
         && e["children"].Size() > 0)
        nested = true;
    }
    CHECK(nested);

    // No name may contain a separator: one that does is a category that was
    // flattened instead of split.
    std::function<void(const rapidjson::Value&)> checkNames
        = [&](const rapidjson::Value& v) {
      const auto name
          = QString::fromUtf8(v["name"].GetString(), v["name"].GetStringLength());
      INFO("library entry: " << name.toStdString());
      CHECK_FALSE(name.contains('/'));

      if(v.HasMember("children"))
        for(const auto& c : v["children"].GetArray())
          checkNames(c);
    };
    for(const auto& e : got.GetArray())
      checkNames(e);
  });
}

TEST_CASE("A terminal is offered the other machine's devices", "[session][terminal]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port, Network::PeerRole::Terminal);
    REQUIRE(client);

    // The dialogs ask the document what may be added; for a terminal that must
    // be the other machine's protocols, not this one's.
    auto& devices = client->context().plugin<Explorer::DeviceDocumentPlugin>();
    auto* catalog = devices.catalog();
    REQUIRE(catalog);

    REQUIRE(spin_until([&] { return !catalog->protocols().empty(); }));

    const auto protocols = catalog->protocols();
    bool named = false;
    for(const auto& p : protocols)
      if(!p.name.isEmpty())
        named = true;
    CHECK(named);

    // Each says whether this build could make one. That is what decides
    // whether a settings form can be shown at all: the widget is C++ in a
    // plug-in, and there is none for a protocol we do not have.
    bool anyConstructible = false;
    for(const auto& p : protocols)
      if(p.constructible)
        anyConstructible = true;
    CHECK(anyConstructible);
  });
}

TEST_CASE("An ordinary document is offered this machine's devices", "[session]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    // The precondition of the case above: a peer that runs the score itself
    // keeps the ordinary dialogs, which read the local factory list directly.
    auto& devices = client->context().plugin<Explorer::DeviceDocumentPlugin>();
    CHECK(devices.catalog() == nullptr);
  });
}

TEST_CASE("A terminal is told which devices are connected", "[session][terminal]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    score::test::register_probe_protocol(ctx);

    auto master = hostSession(ctx);
    auto& hostDevices
        = master.document->context().plugin<Explorer::DeviceDocumentPlugin>();
    hostDevices.explorer().addDevice(
        score::test::probe_device_node(QStringLiteral("stagebox")));

    auto* client = joinSession(ctx, master.port, Network::PeerRole::Terminal);
    REQUIRE(client);

    auto& termDevices = client->context().plugin<Explorer::DeviceDocumentPlugin>();

    // Nothing is behind the node here, so asking a DeviceInterface would say
    // "disconnected" -- which is not unknown, it is wrong. Until the host says
    // otherwise there is no answer at all.
    CHECK_FALSE(termDevices.remoteConnected(QStringLiteral("stagebox")).has_value());

    // What the host reports is what the terminal shows.
    hostDevices.setRemoteConnected(QStringLiteral("ignored"), true);
    master.plugin->policy().session()->broadcastToAllClients(
        master.plugin->policy().session()->makeMessage(
            Network::MessagesAPI::instance().device_status,
            QStringLiteral("stagebox"), true));

    REQUIRE(spin_until([&] {
      return termDevices.remoteConnected(QStringLiteral("stagebox")) == true;
    }));

    master.plugin->policy().session()->broadcastToAllClients(
        master.plugin->policy().session()->makeMessage(
            Network::MessagesAPI::instance().device_status,
            QStringLiteral("stagebox"), false));

    REQUIRE(spin_until([&] {
      return termDevices.remoteConnected(QStringLiteral("stagebox")) == false;
    }));
  });
}
