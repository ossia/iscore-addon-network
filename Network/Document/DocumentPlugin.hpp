#pragma once

#include <score/command/CommandData.hpp>
#include <score/plugins/documentdelegate/plugin/DocumentPlugin.hpp>
#include <score/serialization/DataStreamFwd.hpp>

#include <ossia/detail/hash_map.hpp>

#include <Netpit/Netpit.hpp>
#include <Network/Document/Execution/SyncMode.hpp>

#include <score_addon_network_export.h>

#include <unordered_map>
SCORE_SERIALIZE_DATASTREAM_DECLARE(, score::CommandData)

class DataStream;
class JSONObject;
class QWidget;
struct VisitorVariant;
namespace Process
{
class ProcessModel;
}
namespace Scenario
{
class EventModel;
class IntervalModel;
class TimeSyncModel;
}
namespace Execution
{
class EventComponent;
class IntervalComponent;
class TimeSyncComponent;
}
namespace Netpit
{
struct MessageContext;
struct AudioContext;
struct VideoContext;
}
namespace Network
{
class Group;
class Client;
class NetworkDocumentPlugin;
}
UUID_METADATA(
    , score::DocumentPluginFactory, Network::NetworkDocumentPlugin,
    "58c9e19a-fde3-47d0-a121-35853fec667d")

namespace Network
{

struct ObjectMetadata
{
  std::optional<SyncMode> syncmode{SyncMode::NonCompensatedAsync};
  std::optional<ShareMode> sharemode{ShareMode::Shared};
  QString group;
};

struct NetworkExpressionData
{
  NetworkExpressionData(Execution::TimeSyncComponent& c)
      : component{c}
  {
  }
  Execution::TimeSyncComponent& component;

  //! Will fill itself with received messages
  score::hash_map<Id<Client>, std::optional<bool>> values;
  ossia::hash_set<Id<Client>> previousCompleted;

  Id<Group> thisGroup;
  std::vector<Id<Group>> prevGroups, nextGroups;
  ExpressionPolicy pol;
  SyncMode sync;
  // Trigger : they all have to be set, and true
  // Event : when they are all set, the truth value of each is taken.

  // Expression observation has to be done on the network.
  // Saved in the network components ? For now in the document plugin?

  bool ready(std::size_t count_ready, std::size_t num_clients)
  {
    switch(pol)
    {
      case ExpressionPolicy::OnFirst:
        return count_ready == 1;
      case ExpressionPolicy::OnAll:
        return count_ready >= num_clients; // todo what happens if someone
                                           // disconnects before sending.
      case ExpressionPolicy::OnMajority:
        return count_ready > (num_clients / 2);
      default:
        return false;
    }
  }
};

class Session;
class GroupManager;
class GroupMetadata;
class SCORE_ADDON_NETWORK_EXPORT EditionPolicy : public QObject
{
public:
  using QObject::QObject;
  virtual ~EditionPolicy();
  virtual Session* session() const = 0;
  virtual void play() = 0;
  virtual void stop() = 0;

  bool sendControls() const noexcept { return m_sendControls; }
  void setSendControls(bool b);
  bool sendCommands() const noexcept { return m_sendCommands; }
  void setSendCommands(bool b);

private:
  bool m_sendControls{true};
  bool m_sendCommands{true};
};

class SCORE_ADDON_NETWORK_EXPORT ExecutionPolicy : public QObject
{
  W_OBJECT(ExecutionPolicy)
public:
  using QObject::QObject;
  virtual ~ExecutionPolicy();
  virtual void writeMessage(Netpit::OutboundMessage m) = 0;
  virtual void writeAudio(Netpit::OutboundAudio&& m) = 0;
  virtual void writeVideo(Netpit::OutboundImage&& m) = 0;

  void
  on_message(uint64_t process, const std::vector<std::pair<Id<Client>, ossia::value>>& m)
      W_SIGNAL(on_message, process, m);
  void on_audio(
      uint64_t process,
      const std::vector<std::pair<Id<Client>, std::vector<std::vector<float>>>>& m)
      W_SIGNAL(on_audio, process, m);
  void on_video(uint64_t process, Netpit::InboundImage m) W_SIGNAL(on_video, process, m);
};

class SCORE_ADDON_NETWORK_EXPORT NetworkDocumentPlugin final
    : public score::SerializableDocumentPlugin
{
  W_OBJECT(NetworkDocumentPlugin)

  SCORE_SERIALIZE_FRIENDS
  MODEL_METADATA_IMPL(NetworkDocumentPlugin)
public:
  NetworkDocumentPlugin(
      const score::DocumentContext& ctx, EditionPolicy* policy, QObject* parent);

  virtual ~NetworkDocumentPlugin();

  // Loading has to be in two steps since the plugin policy is different from
  // the client and server.
  NetworkDocumentPlugin(
      const score::DocumentContext& ctx, JSONObject::Deserializer& vis, QObject* parent);
  NetworkDocumentPlugin(
      const score::DocumentContext& ctx, DataStream::Deserializer& vis, QObject* parent);

  void setEditPolicy(EditionPolicy*);
  void setExecPolicy(ExecutionPolicy* e);

  GroupManager& groupManager() const;

  EditionPolicy& policy() const;

  struct NonCompensated
  {
    score::hash_map<Path<Scenario::TimeSyncModel>, std::function<void(Id<Client>)>>
        trigger_evaluation_entered;
    score::hash_map<Path<Scenario::TimeSyncModel>, std::function<void(Id<Client>, bool)>>
        trigger_evaluation_finished;
    score::hash_map<Path<Scenario::TimeSyncModel>, std::function<void(Id<Client>)>>
        trigger_triggered;
    score::hash_map<
        Path<Scenario::IntervalModel>, std::function<void(const Id<Client>&, double)>>
        interval_speed_changed;
    score::hash_map<Path<Scenario::TimeSyncModel>, NetworkExpressionData>
        network_expressions;
  } noncompensated;

  struct Compensated
  {
    score::hash_map<
        Path<Scenario::TimeSyncModel>, std::function<void(Id<Client>, qint64)>>
        trigger_triggered;
  } compensated;

  void on_stop();

  void sessionChanged() W_SIGNAL(sessionChanged);

  ObjectMetadata get_metadata(const Scenario::IntervalModel& obj) const noexcept;
  ObjectMetadata get_metadata(const Scenario::EventModel& obj) const noexcept;
  ObjectMetadata get_metadata(const Scenario::TimeSyncModel& obj) const noexcept;
  ObjectMetadata get_metadata(const Process::ProcessModel& obj) const noexcept;
  void set_metadata(const Scenario::IntervalModel& obj, const ObjectMetadata& m);
  void set_metadata(const Scenario::EventModel& obj, const ObjectMetadata& m);
  void set_metadata(const Scenario::TimeSyncModel& obj, const ObjectMetadata& m);
  void set_metadata(const Process::ProcessModel& obj, const ObjectMetadata& m);
  void unset_metadata(const Scenario::IntervalModel& obj);
  void unset_metadata(const Scenario::EventModel& obj);
  void unset_metadata(const Scenario::TimeSyncModel& obj);
  void unset_metadata(const Process::ProcessModel& obj);

  void register_message_context(std::shared_ptr<Netpit::MessageContext> ctx);
  void unregister_message_context(std::shared_ptr<Netpit::MessageContext> ctx);
  void register_audio_context(std::shared_ptr<Netpit::AudioContext> ctx);
  void unregister_audio_context(std::shared_ptr<Netpit::AudioContext> ctx);
  void register_video_context(std::shared_ptr<Netpit::VideoContext> ctx);
  void unregister_video_context(std::shared_ptr<Netpit::VideoContext> ctx);

  void finish_loading();

private:
  void timerEvent(QTimerEvent* event) override;
  EditionPolicy* m_policy{};
  ExecutionPolicy* m_exec{};
  GroupManager* m_groups{};

  std::unordered_map<uint64_t, std::shared_ptr<Netpit::MessageContext>> m_messages;
  std::unordered_map<uint64_t, std::shared_ptr<Netpit::AudioContext>> m_audio;
  std::unordered_map<uint64_t, std::shared_ptr<Netpit::VideoContext>> m_video;

  int m_timer{};
};
}

W_REGISTER_ARGTYPE(std::vector<std::pair<Id<Network::Client>, ossia::value>>)
W_REGISTER_ARGTYPE(
    std::vector<std::pair<Id<Network::Client>, std::vector<std::vector<float>>>>)
