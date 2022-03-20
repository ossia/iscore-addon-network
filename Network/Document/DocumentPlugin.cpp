#include "DocumentPlugin.hpp"

#include <score/actions/Action.hpp>
#include <score/plugins/documentdelegate/plugin/DocumentPlugin.hpp>
#include <score/plugins/documentdelegate/plugin/DocumentPluginCreator.hpp>
#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>

#include <core/document/Document.hpp>

#include <ossia/editor/expression/expression.hpp>

#include <QObject>

#include <Network/Client/Client.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <Network/Group/Group.hpp>
#include <Network/Group/GroupManager.hpp>
#include <tsl/hopscotch_set.h>

#include <functional>
#include <vector>



#include <Process/Process.hpp>
#include <Scenario/Document/Event/EventModel.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>
#include <Scenario/Process/ScenarioModel.hpp>

#include <score/model/EntityMap.hpp>
#include <score/model/Identifier.hpp>
#include <score/plugins/documentdelegate/plugin/DocumentPlugin.hpp>
#include <score/serialization/VisitorCommon.hpp>
#include <score/tools/std/Optional.hpp>

#include <core/document/Document.hpp>
#include <core/document/DocumentModel.hpp>

#include <QString>

#include <Network/Client/LocalClient.hpp>
#include <Network/Group/Group.hpp>
#include <Network/Group/GroupManager.hpp>
#include <Network/Group/GroupMetadata.hpp>
#include <Network/Group/GroupMetadataWidget.hpp>
#include <Network/Group/Panel/GroupPanelDelegate.hpp>
#include <Network/Session/Session.hpp>
class QWidget;
struct VisitorVariant;

#include <wobjectimpl.h>
W_OBJECT_IMPL(Network::NetworkDocumentPlugin)

SCORE_SERALIZE_DATASTREAM_DEFINE(score::CommandData)

namespace Network
{
MessagesAPI::MessagesAPI()
    : command_new{QByteArrayLiteral("/command/new")}
    , command_undo{QByteArrayLiteral("/command/undo")}
    , command_redo{QByteArrayLiteral("/command/redo")}
    , command_index{QByteArrayLiteral("/command/index")}
    , lock{QByteArrayLiteral("/lock")}
    , unlock{QByteArrayLiteral("/unlock")}
    ,

    ping{QByteArrayLiteral("/ping")}
    , pong{QByteArrayLiteral("/pong")}
    , play{QByteArrayLiteral("/play")}
    , stop{QByteArrayLiteral("/stop")}
    ,

    session_portinfo{QByteArrayLiteral("/session/portinfo")}
    , session_askNewId{QByteArrayLiteral("/session/askNewId")}
    , session_idOffer{QByteArrayLiteral("/session/idOffer")}
    , session_join{QByteArrayLiteral("/session/join")}
    , session_document{QByteArrayLiteral("/session/document")}
    ,

    trigger_expression_true{QByteArrayLiteral("/trigger/expression_true")}
    , trigger_previous_completed{QByteArrayLiteral(
          "/trigger/previous_completed")}
    , trigger_entered{QByteArrayLiteral("/trigger/entered")}
    , trigger_left{QByteArrayLiteral("/trigger/left")}
    , trigger_finished{QByteArrayLiteral("/trigger/finished")}
    , trigger_triggered{QByteArrayLiteral("/trigger/triggered")}
    , interval_speed{QByteArrayLiteral("/interval/speed")}
{
}

const MessagesAPI& MessagesAPI::instance()
{
  static const MessagesAPI api;
  return api;
}

NetworkDocumentPlugin::NetworkDocumentPlugin(
    const score::DocumentContext& ctx,
    EditionPolicy* policy,
    QObject* parent)
    : score::SerializableDocumentPlugin{ctx,
                                        "NetworkDocumentPlugin",
                                        parent}
    , m_policy{policy}
    , m_groups{new GroupManager{this}}
{
  SCORE_ASSERT(policy);
  m_policy->setParent(this);

  // Base group set-up
  auto allGroup = new Group{"all", Id<Group>{0}, &groupManager()};
  allGroup->addClient(m_policy->session()->localClient().id());
  groupManager().addGroup(allGroup);
}

NetworkDocumentPlugin::~NetworkDocumentPlugin() {}

NetworkDocumentPlugin::NetworkDocumentPlugin(const score::DocumentContext& ctx, JSONObject::Deserializer& vis, QObject* parent)
  : score::SerializableDocumentPlugin{ctx, vis, parent}
{
  vis.writeTo(*this);
}

NetworkDocumentPlugin::NetworkDocumentPlugin(const score::DocumentContext& ctx, DataStream::Deserializer& vis, QObject* parent)
  : score::SerializableDocumentPlugin{ctx, vis, parent}
{
  vis.writeTo(*this);
}

void NetworkDocumentPlugin::setEditPolicy(EditionPolicy* pol)
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

GroupManager& NetworkDocumentPlugin::groupManager() const { return *m_groups; }

EditionPolicy& NetworkDocumentPlugin::policy() const { return *m_policy; }

void NetworkDocumentPlugin::on_stop()
{
  noncompensated.trigger_evaluation_entered.clear();
  noncompensated.trigger_evaluation_finished.clear();
  noncompensated.trigger_triggered.clear();
  noncompensated.interval_speed_changed.clear();
  noncompensated.network_expressions.clear();

  compensated.trigger_triggered.clear();
}

const ObjectMetadata* NetworkDocumentPlugin::get_metadata(const Scenario::IntervalModel& obj)
{
  if(auto it = intervalMetadatas().find(&obj); it != intervalMetadatas().end())
    return &it->second;
  return {};
}

const ObjectMetadata* NetworkDocumentPlugin::get_metadata(const Scenario::EventModel& obj)
{
  if(auto it = eventMetadatas().find(&obj); it != eventMetadatas().end())
    return &it->second;
  return {};
}

const ObjectMetadata* NetworkDocumentPlugin::get_metadata(const Scenario::TimeSyncModel& obj)
{
  if(auto it = syncMetadatas().find(&obj); it != syncMetadatas().end())
    return &it->second;
  return {};
}

const ObjectMetadata* NetworkDocumentPlugin::get_metadata(const Process::ProcessModel& obj)
{
  if(auto it = processMetadatas().find(&obj); it != processMetadatas().end())
    return &it->second;
  return {};
}

void NetworkDocumentPlugin::set_metadata(const Scenario::IntervalModel& obj, const ObjectMetadata& m)
{

}

const std::unordered_map<const Scenario::IntervalModel*, ObjectMetadata>& NetworkDocumentPlugin::intervalMetadatas() const noexcept
{ return m_intervalsGroups; }

const std::unordered_map<const Scenario::EventModel*, ObjectMetadata>& NetworkDocumentPlugin::eventMetadatas() const noexcept
{ return m_eventGroups; }

const std::unordered_map<const Scenario::TimeSyncModel*, ObjectMetadata>& NetworkDocumentPlugin::syncMetadatas() const noexcept
{ return m_syncGroups; }

const std::unordered_map<const Process::ProcessModel*, ObjectMetadata>& NetworkDocumentPlugin::processMetadatas() const noexcept
{ return m_processGroups; }

ExecutionPolicy::~ExecutionPolicy() {}

EditionPolicy::~EditionPolicy() {}
}
