#pragma once
// The two halves of a session, as two documents in one process. Shared by the
// headless replication tests and the ones that need the GUI stack, which
// cannot live in the same binary.

#include <Scenario/Document/BaseScenario/BaseScenario.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>
#include <Scenario/Commands/Metadata/ChangeElementLabel.hpp>

#include <score/application/ApplicationContext.hpp>
#include <score/document/DocumentContext.hpp>
#include <score/tools/IdentifierGeneration.hpp>

#include <core/command/CommandStack.hpp>
#include <core/document/Document.hpp>
#include <core/document/DocumentModel.hpp>
#include <core/presenter/DocumentManager.hpp>

#include <QApplication>
#include <QElapsedTimer>

#include <Network/Client/LocalClient.hpp>
#include <Network/Communication/Capabilities.hpp>
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Document/MasterPolicy.hpp>
#include <Network/Session/ClientSessionBuilder.hpp>
#include <Network/Session/MasterSession.hpp>
#include <Network/Client/PeerRole.hpp>

#include <score_test/Document.hpp>

#include <memory>

namespace SessionTest
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

inline Scenario::IntervalModel& rootInterval(score::Document& doc)
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
inline Master hostSession(const score::GUIApplicationContext& ctx)
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

inline Network::Capabilities* g_lastMasterCapabilities{};

//! Join a session as a second document in this same process.
//!
//! Polls rather than connecting to the builder's signals: verdigris signals do
//! not resolve by member-pointer across a shared-library boundary, since the
//! metaobject's IndexOfMethod handler is not exported.
inline score::Document* joinSession(
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
