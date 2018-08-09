#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>
#include <Scenario/Process/ScenarioModel.hpp>
#include <score/tools/std/Optional.hpp>
#include <core/document/Document.hpp>
#include <core/document/DocumentModel.hpp>
#include <score/serialization/VisitorCommon.hpp>

#include <QString>

#include <Network/Group/Group.hpp>
#include <Network/Group/GroupManager.hpp>
#include <Network/Group/GroupMetadata.hpp>
#include <Network/Group/GroupMetadataWidget.hpp>
#include "DocumentPlugin.hpp"
#include <Process/Process.hpp>
#include <Network/Session/Session.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/Event/EventModel.hpp>
#include <score/plugins/documentdelegate/plugin/DocumentPlugin.hpp>
#include <score/model/EntityMap.hpp>
#include <score/model/Identifier.hpp>
#include <Network/Client/LocalClient.hpp>
#include <Network/Group/Panel/GroupPanelDelegate.hpp>
class QWidget;
struct VisitorVariant;

#include <wobjectimpl.h>
W_OBJECT_IMPL(Network::NetworkDocumentPlugin)
namespace Network
{
MessagesAPI::MessagesAPI():
  command_new{QByteArrayLiteral("/command/new")},
  command_undo{QByteArrayLiteral("/command/undo")},
  command_redo{QByteArrayLiteral("/command/redo")},
  command_index{QByteArrayLiteral("/command/index")},
  lock{QByteArrayLiteral("/lock")},
  unlock{QByteArrayLiteral("/unlock")},

  ping{QByteArrayLiteral("/ping")},
  pong{QByteArrayLiteral("/pong")},
  play{QByteArrayLiteral("/play")},
  stop{QByteArrayLiteral("/stop")},

  session_portinfo{QByteArrayLiteral("/session/portinfo")},
  session_askNewId{QByteArrayLiteral("/session/askNewId")},
  session_idOffer{QByteArrayLiteral("/session/idOffer")},
  session_join{QByteArrayLiteral("/session/join")},
  session_document{QByteArrayLiteral("/session/document")},

  trigger_expression_true{QByteArrayLiteral("/trigger/expression_true")},
  trigger_previous_completed{QByteArrayLiteral("/trigger/previous_completed")},
  trigger_entered{QByteArrayLiteral("/trigger/entered")},
  trigger_left{QByteArrayLiteral("/trigger/left")},
  trigger_finished{QByteArrayLiteral("/trigger/finished")},
  trigger_triggered{QByteArrayLiteral("/trigger/triggered")},
  interval_speed{QByteArrayLiteral("/interval/speed")}
{

}

const MessagesAPI& MessagesAPI::instance()
{
  static const MessagesAPI api;
  return api;
}

NetworkDocumentPlugin::NetworkDocumentPlugin(
    const score::DocumentContext& ctx,
    EditionPolicy *policy,
    Id<score::DocumentPlugin> id,
    QObject* parent):
  score::SerializableDocumentPlugin{ctx, std::move(id), "NetworkDocumentPlugin", parent},
  m_policy{policy},
  m_groups{new GroupManager{this}}
{
  SCORE_ASSERT(policy);
  m_policy->setParent(this);

  // Base group set-up
  auto allGroup = new Group{"all", Id<Group>{0}, &groupManager()};
  allGroup->addClient(m_policy->session()->localClient().id());
  groupManager().addGroup(allGroup);
}

NetworkDocumentPlugin::~NetworkDocumentPlugin()
{

}

void NetworkDocumentPlugin::setEditPolicy(EditionPolicy * pol)
{
  SCORE_ASSERT(pol);

  delete m_policy;
  pol->setParent(this);
  m_policy = pol;
  m_groups->cleanup(m_policy->session()->remoteClients());

  sessionChanged();
}

void NetworkDocumentPlugin::setExecPolicy(ExecutionPolicy* pol)
{
  SCORE_ASSERT(pol);

  delete m_exec;
  pol->setParent(this);
  m_exec = pol;
}

void NetworkDocumentPlugin::on_stop()
{
  noncompensated.trigger_evaluation_entered.clear();
  noncompensated.trigger_evaluation_finished.clear();
  noncompensated.trigger_triggered.clear();
  noncompensated.interval_speed_changed.clear();
  noncompensated.network_expressions.clear();

  compensated.trigger_triggered.clear();
}

score::DocumentPlugin*DocumentPluginFactory::load(
    const VisitorVariant& var,
    score::DocumentContext& doc,
    QObject* parent)
{
  return score::deserialize_dyn(var, [&] (auto&& deserializer)
  { return new NetworkDocumentPlugin{doc, deserializer, parent}; });
}

ExecutionPolicy::~ExecutionPolicy()
{

}

EditionPolicy::~EditionPolicy()
{

}

}

