#include "DocumentPlugin.hpp"

#include <Process/Process.hpp>

#include <Scenario/Document/Event/EventModel.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>
#include <Scenario/Process/ScenarioModel.hpp>

#include <score/actions/Action.hpp>
#include <score/model/EntityMap.hpp>
#include <score/model/Identifier.hpp>
#include <score/plugins/documentdelegate/plugin/DocumentPlugin.hpp>
#include <score/plugins/documentdelegate/plugin/DocumentPluginCreator.hpp>
#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>
#include <score/serialization/VisitorCommon.hpp>
#include <score/tools/std/Optional.hpp>

#include <core/document/Document.hpp>
#include <core/document/DocumentModel.hpp>

#include <ossia/detail/hash_map.hpp>
#include <ossia/editor/expression/expression.hpp>

#include <QObject>
#include <QString>

#include <Netpit/MessageContext.hpp>
#include <Network/Client/Client.hpp>
#include <Network/Client/RemoteClient.hpp>
#include <Network/Document/DeviceQueries.hpp>
#include <Network/Document/FileQueries.hpp>
#include <Network/Document/ObjectQueries.hpp>
#include <Network/Document/LibraryQueries.hpp>
#include <Network/Client/LocalClient.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <Network/Group/Group.hpp>
#include <Network/Group/GroupManager.hpp>
#include <Network/Group/GroupMetadata.hpp>
#include <Network/Group/GroupMetadataWidget.hpp>
#include <Network/Group/Panel/GroupPanelDelegate.hpp>
#include <Network/Session/Session.hpp>

#include <functional>
#include <vector>
class QWidget;
struct VisitorVariant;

#include <wobjectimpl.h>
W_OBJECT_IMPL(Network::NetworkDocumentPlugin)
W_OBJECT_IMPL(Network::ExecutionPolicy)

SCORE_SERALIZE_DATASTREAM_DEFINE(score::CommandData)

namespace Network
{
MessagesAPI::MessagesAPI()
    : command_new{QByteArrayLiteral("/command/new")}
    , command_undo{QByteArrayLiteral("/command/undo")}
    , command_redo{QByteArrayLiteral("/command/redo")}
    , command_index{QByteArrayLiteral("/command/index")}
    , command_rejected{QByteArrayLiteral("/command/rejected")}
    , rpc_request{QByteArrayLiteral("/rpc/request")}
    , rpc_response{QByteArrayLiteral("/rpc/response")}
    , lock{QByteArrayLiteral("/lock")}
    , unlock{QByteArrayLiteral("/unlock")}
    ,

    exec_position{QByteArrayLiteral("/exec/position")}
    , ping{QByteArrayLiteral("/ping")}
    , pong{QByteArrayLiteral("/pong")}
    , play{QByteArrayLiteral("/play")}
    , stop{QByteArrayLiteral("/stop")}
    ,

    session_portinfo{QByteArrayLiteral("/session/portinfo")}
    , session_askNewId{QByteArrayLiteral("/session/askNewId")}
    , session_idOffer{QByteArrayLiteral("/session/idOffer")}
    , session_join{QByteArrayLiteral("/session/join")}
    , session_document{QByteArrayLiteral("/session/document")}
    , session_rejected{QByteArrayLiteral("/session/rejected")}
    ,

    trigger_expression_true{QByteArrayLiteral("/trigger/expression_true")}
    , trigger_previous_completed{QByteArrayLiteral("/trigger/previous_completed")}
    , trigger_entered{QByteArrayLiteral("/trigger/entered")}
    , trigger_left{QByteArrayLiteral("/trigger/left")}
    , trigger_finished{QByteArrayLiteral("/trigger/finished")}
    , trigger_triggered{QByteArrayLiteral("/trigger/triggered")}
    , interval_speed{QByteArrayLiteral("/interval/speed")}

    , netpit_in_message{QByteArrayLiteral("/np/msg/in")}
    , netpit_out_message{QByteArrayLiteral("/np/msg/out")}
    , netpit_in_audio{QByteArrayLiteral("/np/audio/in")}
    , netpit_out_audio{QByteArrayLiteral("/np/audio/out")}
    , netpit_in_video{QByteArrayLiteral("/np/video/in")}
    , netpit_out_video{QByteArrayLiteral("/np/video/out")}
    , netpit_in_geometry{QByteArrayLiteral("/np/geom/in")}
    , netpit_out_geometry{QByteArrayLiteral("/np/geom/out")}
{
}

const MessagesAPI& MessagesAPI::instance()
{
  static const MessagesAPI api;
  return api;
}

NetworkDocumentPlugin::NetworkDocumentPlugin(
    const score::DocumentContext& ctx, EditionPolicy* policy, QObject* parent)
    : score::SerializableDocumentPlugin{ctx, "NetworkDocumentPlugin", parent}
    , m_policy{policy}
    , m_groups{new GroupManager{this}}
{
  SCORE_ASSERT(policy);
  m_policy->setParent(this);

  m_rpc = std::make_unique<RpcChannel>(*m_policy->session());
  bindDeviceQueries(*m_rpc, m_context);
  bindFileQueries(*m_rpc, m_context);
  bindObjectQueries(*m_rpc, m_context);
  bindLibraryQueries(*m_rpc, m_context);

  // A question put to a peer that then leaves would otherwise wait out its
  // timeout with nothing to wait for.
  connect(
      m_policy->session(), &Session::clientRemoved, m_rpc.get(),
      [this](RemoteClient* c) {
    if(c && m_rpc)
      m_rpc->peerLost(c->id());
      });

  // Base group set-up
  auto allGroup = new Group{"all", Id<Group>{0}, &groupManager()};
  allGroup->addClient(m_policy->session()->localClient().id());
  groupManager().addGroup(allGroup);
}

NetworkDocumentPlugin::~NetworkDocumentPlugin() { }

NetworkDocumentPlugin::NetworkDocumentPlugin(
    const score::DocumentContext& ctx, JSONObject::Deserializer& vis, QObject* parent)
    : score::SerializableDocumentPlugin{ctx, vis, parent}
{
  vis.writeTo(*this);
}

NetworkDocumentPlugin::NetworkDocumentPlugin(
    const score::DocumentContext& ctx, DataStream::Deserializer& vis, QObject* parent)
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

  // Its handlers live on the session's mapper, so it belongs to whichever
  // session we are now part of rather than to the one we just left.
  m_rpc = std::make_unique<RpcChannel>(*m_policy->session());
  bindDeviceQueries(*m_rpc, m_context);
  bindFileQueries(*m_rpc, m_context);
  bindObjectQueries(*m_rpc, m_context);
  bindLibraryQueries(*m_rpc, m_context);

  // A question put to a peer that then leaves would otherwise wait out its
  // timeout with nothing to wait for.
  connect(
      m_policy->session(), &Session::clientRemoved, m_rpc.get(),
      [this](RemoteClient* c) {
    if(c && m_rpc)
      m_rpc->peerLost(c->id());
      });

  m_groups->cleanup(m_policy->session()->remoteClients());

  sessionChanged();
}

void NetworkDocumentPlugin::setExecPolicy(ExecutionPolicy* pol)
{
  SCORE_ASSERT(pol);

  delete m_exec;
  killTimer(m_timer);
  if(pol)
  {
    pol->setParent(this);
    m_exec = pol;

    m_timer = startTimer(4);
    connect(
        m_exec, &ExecutionPolicy::on_message, this,
        [this](uint64_t process, std::vector<std::pair<Id<Client>, ossia::value>> m) {
      // Process messages
      auto it = m_messages.find(process);
      if(it != m_messages.end())
      {
        auto& p = it->second;
        if(p)
        {
          Netpit::InboundMessages vec;
          vec.reserve(m.size());
          for(auto& [i, v] : m)
            vec.emplace_back(Netpit::InboundMessage{std::move(v), i.val()});

          p->from_network.enqueue(std::move(vec));
        }
      }
        });

    connect(
        m_exec, &ExecutionPolicy::on_audio, this,
        [this](
            uint64_t process,
            std::vector<std::pair<Id<Client>, std::vector<std::vector<float>>>> m) {
      // Process audio
      auto it = m_audio.find(process);
      if(it != m_audio.end())
      {
        auto& p = it->second;
        if(p)
        {
          Netpit::InboundAudios vec;
          vec.reserve(m.size());
          for(auto& [i, v] : m)
          {
            vec.emplace_back(Netpit::InboundAudio{std::move(v), i.val()});
          }

          p->from_network.enqueue(std::move(vec));
        }
      }
        });

    connect(
        m_exec, &ExecutionPolicy::on_video, this,
        [this](uint64_t process, Netpit::InboundImage m) {
      // Process video
      auto it = m_video.find(process);
      if(it != m_video.end())
      {
        auto& p = it->second;
        if(p)
        {
          p->from_network.enqueue(std::move(m));
        }
      }
        });
  }
}

GroupManager& NetworkDocumentPlugin::groupManager() const
{
  return *m_groups;
}

EditionPolicy& NetworkDocumentPlugin::policy() const
{
  return *m_policy;
}

void NetworkDocumentPlugin::setRemoteCapabilities(Capabilities c)
{
  m_remoteCaps = std::move(c);
}

void NetworkDocumentPlugin::setDiverged(const QString& reason)
{
  // Only the first divergence is meaningful: everything after it is a
  // consequence of applying the stream to a model that no longer matches.
  if(!m_divergence.isEmpty())
    return;

  m_divergence = reason;

  // Stop sending as well as stop applying. Our edits are now expressed against
  // a document nobody else has, and the paths in them name different objects
  // on the other side: one command we could not apply would otherwise become
  // open-ended corruption of everybody else's copy.
  if(m_policy)
    m_policy->setSendCommands(false);

  qWarning() << "Network session diverged:" << reason
             << "- this document no longer matches the session and must be "
                "rejoined to edit it safely.";
  divergedChanged(reason);
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

const ObjectMetadata*
NetworkDocumentPlugin::get_metadata(const Scenario::IntervalModel& obj) const noexcept
{
  if(auto it = m_intervalsGroups.find(&obj); it != m_intervalsGroups.end())
    return &it->second;
  return {};
}

const ObjectMetadata*
NetworkDocumentPlugin::get_metadata(const Scenario::EventModel& obj) const noexcept
{
  if(auto it = m_eventGroups.find(&obj); it != m_eventGroups.end())
    return &it->second;
  return {};
}

const ObjectMetadata*
NetworkDocumentPlugin::get_metadata(const Scenario::TimeSyncModel& obj) const noexcept
{
  if(auto it = m_syncGroups.find(&obj); it != m_syncGroups.end())
    return &it->second;
  return {};
}

const ObjectMetadata*
NetworkDocumentPlugin::get_metadata(const Process::ProcessModel& obj) const noexcept
{
  if(auto it = m_processGroups.find(&obj); it != m_processGroups.end())
    return &it->second;
  return {};
}
ObjectMetadata*
NetworkDocumentPlugin::get_metadata(const Scenario::IntervalModel& obj) noexcept
{
  if(auto it = m_intervalsGroups.find(&obj); it != m_intervalsGroups.end())
    return &it->second;
  return {};
}

ObjectMetadata*
NetworkDocumentPlugin::get_metadata(const Scenario::EventModel& obj) noexcept
{
  if(auto it = m_eventGroups.find(&obj); it != m_eventGroups.end())
    return &it->second;
  return {};
}

ObjectMetadata*
NetworkDocumentPlugin::get_metadata(const Scenario::TimeSyncModel& obj) noexcept
{
  if(auto it = m_syncGroups.find(&obj); it != m_syncGroups.end())
    return &it->second;
  return {};
}

ObjectMetadata*
NetworkDocumentPlugin::get_metadata(const Process::ProcessModel& obj) noexcept
{
  if(auto it = m_processGroups.find(&obj); it != m_processGroups.end())
    return &it->second;
  return {};
}

void NetworkDocumentPlugin::set_metadata(
    const Scenario::IntervalModel& obj, const ObjectMetadata& m)
{
  m_intervalsGroups[&obj] = m;
  connect(
      &obj, &IdentifiedObjectAbstract::identified_object_destroying, this,
      [this, ptr = &obj] { m_intervalsGroups.erase(ptr); });
}

void NetworkDocumentPlugin::set_metadata(
    const Scenario::EventModel& obj, const ObjectMetadata& m)
{
  m_eventGroups[&obj] = m;
  connect(
      &obj, &IdentifiedObjectAbstract::identified_object_destroying, this,
      [this, ptr = &obj] { m_eventGroups.erase(ptr); });
}

void NetworkDocumentPlugin::set_metadata(
    const Scenario::TimeSyncModel& obj, const ObjectMetadata& m)
{
  m_syncGroups[&obj] = m;
  connect(
      &obj, &IdentifiedObjectAbstract::identified_object_destroying, this,
      [this, ptr = &obj] { m_syncGroups.erase(ptr); });
}

void NetworkDocumentPlugin::set_metadata(
    const Process::ProcessModel& obj, const ObjectMetadata& m)
{
  m_processGroups[&obj] = m;
  connect(
      &obj, &IdentifiedObjectAbstract::identified_object_destroying, this,
      [this, ptr = &obj] { m_processGroups.erase(ptr); });
}
void NetworkDocumentPlugin::unset_metadata(const Scenario::IntervalModel& obj)
{
  m_intervalsGroups.erase(&obj);
  disconnect(
      &obj, &IdentifiedObjectAbstract::identified_object_destroying, this, nullptr);
}

void NetworkDocumentPlugin::unset_metadata(const Scenario::EventModel& obj)
{
  m_eventGroups.erase(&obj);
  disconnect(
      &obj, &IdentifiedObjectAbstract::identified_object_destroying, this, nullptr);
}

void NetworkDocumentPlugin::unset_metadata(const Scenario::TimeSyncModel& obj)
{
  m_syncGroups.erase(&obj);
  disconnect(
      &obj, &IdentifiedObjectAbstract::identified_object_destroying, this, nullptr);
}

void NetworkDocumentPlugin::unset_metadata(const Process::ProcessModel& obj)
{
  m_processGroups.erase(&obj);
  disconnect(
      &obj, &IdentifiedObjectAbstract::identified_object_destroying, this, nullptr);
}

void NetworkDocumentPlugin::register_message_context(
    std::shared_ptr<Netpit::MessageContext> ctx)
{
  m_messages[ctx->instance] = std::move(ctx);
}

void NetworkDocumentPlugin::unregister_message_context(
    std::shared_ptr<Netpit::MessageContext> ctx)
{
  m_messages.erase(ctx->instance);
}

void NetworkDocumentPlugin::register_audio_context(
    std::shared_ptr<Netpit::AudioContext> ctx)
{
  m_audio[ctx->instance] = std::move(ctx);
}

void NetworkDocumentPlugin::unregister_audio_context(
    std::shared_ptr<Netpit::AudioContext> ctx)
{
  m_audio.erase(ctx->instance);
}

void NetworkDocumentPlugin::register_video_context(
    std::shared_ptr<Netpit::VideoContext> ctx)
{
  m_video[ctx->instance] = std::move(ctx);
}

void NetworkDocumentPlugin::unregister_video_context(
    std::shared_ptr<Netpit::VideoContext> ctx)
{
  m_video.erase(ctx->instance);
}

void NetworkDocumentPlugin::finish_loading()
{
  auto& ctx = this->context();
  for(auto& [p, m] : m_loadIntervalsGroups)
    m_intervalsGroups[&p.find(ctx)] = std::move(m);
  for(auto& [p, m] : m_loadEventGroups)
    m_eventGroups[&p.find(ctx)] = std::move(m);
  for(auto& [p, m] : m_loadSyncGroups)
    m_syncGroups[&p.find(ctx)] = std::move(m);
  for(auto& [p, m] : m_loadProcessGroups)
    m_processGroups[&p.find(ctx)] = std::move(m);

  m_loadIntervalsGroups.clear();
  m_loadIntervalsGroups.shrink_to_fit();
  m_loadEventGroups.clear();
  m_loadEventGroups.shrink_to_fit();
  m_loadSyncGroups.clear();
  m_loadSyncGroups.shrink_to_fit();
  m_loadProcessGroups.clear();
  m_loadProcessGroups.shrink_to_fit();
}

const std::unordered_map<const Scenario::IntervalModel*, ObjectMetadata>&
NetworkDocumentPlugin::intervalMetadatas() const noexcept
{
  return m_intervalsGroups;
}

const std::unordered_map<const Scenario::EventModel*, ObjectMetadata>&
NetworkDocumentPlugin::eventMetadatas() const noexcept
{
  return m_eventGroups;
}

const std::unordered_map<const Scenario::TimeSyncModel*, ObjectMetadata>&
NetworkDocumentPlugin::syncMetadatas() const noexcept
{
  return m_syncGroups;
}

const std::unordered_map<const Process::ProcessModel*, ObjectMetadata>&
NetworkDocumentPlugin::processMetadatas() const noexcept
{
  return m_processGroups;
}

ExecutionPolicy::~ExecutionPolicy() { }

EditionPolicy::~EditionPolicy() { }

void EditionPolicy::setSendControls(bool b)
{
  m_sendControls = b;
}
void EditionPolicy::setSendCommands(bool b)
{
  m_sendCommands = b;
}
}

void Network::NetworkDocumentPlugin::timerEvent(QTimerEvent* event)
{
  if(!m_exec)
    return;

  // Send all the messages written by the local processes to the network
  for(auto& m : m_messages)
  {
    Netpit::MessageContext& ctx = *m.second;
    Netpit::OutboundMessage mm;
    bool ok = false;
    while(ctx.to_network.try_dequeue(mm))
      ok = true;
    if(ok)
    {
      m_exec->writeMessage(mm);
    }
  }

  for(auto& m : m_audio)
  {
    Netpit::AudioContext& ctx = *m.second;
    Netpit::OutboundAudio mm;
    while(ctx.to_network.try_dequeue(mm))
      m_exec->writeAudio(std::move(mm));
  }

  for(auto& m : m_video)
  {
    Netpit::VideoContext& ctx = *m.second;
    Netpit::OutboundImage mm;
    while(ctx.to_network.try_dequeue(mm))
      m_exec->writeVideo(std::move(mm));
  }
}
