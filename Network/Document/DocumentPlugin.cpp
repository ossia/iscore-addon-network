#include "DocumentPlugin.hpp"

#include <Process/Commands/SetControlValue.hpp>
#include <Process/Process.hpp>

#include <Scenario/Document/Event/EventModel.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/NetworkMetadata.hpp>
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
    , lock{QByteArrayLiteral("/lock")}
    , unlock{QByteArrayLiteral("/unlock")}
    ,

    ping{QByteArrayLiteral("/ping")}
    , pong{QByteArrayLiteral("/pong")}
    , play{QByteArrayLiteral("/play")}
    , stop{QByteArrayLiteral("/stop")}
    ,

    session_portinfo{QByteArrayLiteral("/session/portinfo")}
    , session_newclientinfo{QByteArrayLiteral("/session/newclientinfo")}
    , session_askNewId{QByteArrayLiteral("/session/askNewId")}
    , session_idOffer{QByteArrayLiteral("/session/idOffer")}
    , session_join{QByteArrayLiteral("/session/join")}
    , session_document{QByteArrayLiteral("/session/document")}
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

void NetworkDocumentPlugin::on_stop()
{
  noncompensated.trigger_evaluation_entered.clear();
  noncompensated.trigger_evaluation_finished.clear();
  noncompensated.trigger_triggered.clear();
  noncompensated.interval_speed_changed.clear();
  noncompensated.network_expressions.clear();

  compensated.trigger_triggered.clear();
}
static auto from_net_metadata(auto& obj, const ObjectMetadata& meta)
{
  obj.setNetworkGroup(meta.group);
  Process::NetworkFlags flags{};
  if(meta.syncmode)
  {
    switch(*meta.syncmode)
    {
      case SyncMode::NonCompensatedAsync:
        flags |= Process::NetworkFlags::Uncompensated | Process::NetworkFlags::Async;
        break;
      case SyncMode::CompensatedAsync:
        flags |= Process::NetworkFlags::Compensated | Process::NetworkFlags::Async;
        break;
      case SyncMode::NonCompensatedSync:
        flags |= Process::NetworkFlags::Uncompensated | Process::NetworkFlags::Sync;
        break;
      case SyncMode::CompensatedSync:
        flags |= Process::NetworkFlags::Compensated | Process::NetworkFlags::Sync;
        break;
    }
  }

  if(meta.sharemode)
  {
    switch(*meta.sharemode)
    {
      case ShareMode::Free:
        flags |= Process::NetworkFlags::Free;
        break;
      case ShareMode::Mixed:
        flags |= Process::NetworkFlags::Mixed;
        break;
      case ShareMode::Shared:
        flags |= Process::NetworkFlags::Shared;
        break;
    }
  }

  if(obj.networkFlags() & Process::NetworkFlags::Active)
    flags |= Process::NetworkFlags::Active;

  obj.setNetworkFlags(flags);
}

static auto to_net_metadata(auto& obj)
{
  ObjectMetadata m;
  m.group = Scenario::networkGroup(obj);
  const auto f = Scenario::networkFlags(obj);
  if(f & Process::NetworkFlags::Sync)
  {
    if(f & Process::NetworkFlags::Compensated)
      m.syncmode = SyncMode::CompensatedSync;
    else
      m.syncmode = SyncMode::NonCompensatedSync;
  }
  else
  {
    if(f & Process::NetworkFlags::Compensated)
      m.syncmode = SyncMode::CompensatedAsync;
    else
      m.syncmode = SyncMode::NonCompensatedAsync;
  }

  if(f & Process::NetworkFlags::Shared)
  {
    m.sharemode = ShareMode::Shared;
  }
  else if(f & Process::NetworkFlags::Mixed)
  {
    m.sharemode = ShareMode::Mixed;
  }
  else if(f & Process::NetworkFlags::Free)
  {
    m.sharemode = ShareMode::Free;
  }

  return m;
}
ObjectMetadata
NetworkDocumentPlugin::get_metadata(const Scenario::IntervalModel& obj) const noexcept
{
  return to_net_metadata(obj);
}

ObjectMetadata
NetworkDocumentPlugin::get_metadata(const Scenario::EventModel& obj) const noexcept
{
  return to_net_metadata(obj);
}

ObjectMetadata
NetworkDocumentPlugin::get_metadata(const Scenario::TimeSyncModel& obj) const noexcept
{
  return to_net_metadata(obj);
}

ObjectMetadata
NetworkDocumentPlugin::get_metadata(const Process::ProcessModel& obj) const noexcept
{
  return to_net_metadata(obj);
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

void NetworkDocumentPlugin::finish_loading() { }

ExecutionPolicy::~ExecutionPolicy() { }

EditionPolicy::EditionPolicy(const score::DocumentContext& ctx, QObject* parent)
    : QObject{parent}
    , m_ctx{ctx}
{
}
EditionPolicy::~EditionPolicy() { }

void EditionPolicy::setSendControls(bool b)
{
  m_sendControls = b;
}
void EditionPolicy::setSendCommands(bool b)
{
  m_sendCommands = b;
}

bool EditionPolicy::canSendCommand(score::Command* cmd) const
{
  using namespace std::literals;
  if(!this->sendCommands())
    return false;
  if(cmd->key().toString() != "SetControlValue"sv)
    return true;
  if(this->sendControls())
  {
    auto ccmd = safe_cast<Process::SetControlValue*>(cmd);
    auto proc = qobject_cast<Process::ProcessModel*>(ccmd->path().find(m_ctx).parent());
    auto flags = Scenario::networkFlags(*proc);
    if(flags & Process::NetworkFlags::Shared)
    {
      return true;
    }
  }
  return false;
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
