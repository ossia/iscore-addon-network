#pragma once

#include <score/plugins/documentdelegate/plugin/DocumentPlugin.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <score_addon_network_export.h>
#include <tsl/hopscotch_set.h>
#include <score/serialization/DataStreamFwd.hpp>
#include <score/command/CommandData.hpp>
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
namespace Network
{
class Group;
class Client;
class NetworkDocumentPlugin;
}
UUID_METADATA(
    ,
    score::DocumentPluginFactory,
    Network::NetworkDocumentPlugin,
    "58c9e19a-fde3-47d0-a121-35853fec667d")

namespace Network
{

struct ObjectMetadata
{
    SyncMode syncmode{SyncMode::NonCompensatedAsync};
    ShareMode sharemode{ShareMode::Shared};
    QString group;
};

struct NetworkExpressionData
{
  NetworkExpressionData(Execution::TimeSyncComponent& c) : component{c} {}
  Execution::TimeSyncComponent& component;

  //! Will fill itself with received messages
  score::hash_map<Id<Client>, std::optional<bool>> values;
  tsl::hopscotch_set<Id<Client>> previousCompleted;

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
    switch (pol)
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
};

class SCORE_ADDON_NETWORK_EXPORT ExecutionPolicy : public QObject
{
public:
  using QObject::QObject;
  virtual ~ExecutionPolicy();
};

class SCORE_ADDON_NETWORK_EXPORT NetworkDocumentPlugin final
    : public score::SerializableDocumentPlugin
{
  W_OBJECT(NetworkDocumentPlugin)

  SCORE_SERIALIZE_FRIENDS
  MODEL_METADATA_IMPL(NetworkDocumentPlugin)
public:
  NetworkDocumentPlugin(
      const score::DocumentContext& ctx,
      EditionPolicy* policy,
      QObject* parent);

  virtual ~NetworkDocumentPlugin();

  // Loading has to be in two steps since the plugin policy is different from
  // the client and server.
  NetworkDocumentPlugin(
      const score::DocumentContext& ctx,
      JSONObject::Deserializer& vis,
      QObject* parent);
  NetworkDocumentPlugin(
        const score::DocumentContext& ctx,
        DataStream::Deserializer& vis,
        QObject* parent);

  void setEditPolicy(EditionPolicy*);
  void setExecPolicy(ExecutionPolicy* e);

  GroupManager& groupManager() const;

  EditionPolicy& policy() const;

  struct NonCompensated
  {
    score::hash_map<
        Path<Scenario::TimeSyncModel>,
        std::function<void(Id<Client>)>>
        trigger_evaluation_entered;
    score::hash_map<
        Path<Scenario::TimeSyncModel>,
        std::function<void(Id<Client>, bool)>>
        trigger_evaluation_finished;
    score::hash_map<
        Path<Scenario::TimeSyncModel>,
        std::function<void(Id<Client>)>>
        trigger_triggered;
    score::hash_map<
        Path<Scenario::IntervalModel>,
        std::function<void(const Id<Client>&, double)>>
        interval_speed_changed;
    score::hash_map<Path<Scenario::TimeSyncModel>, NetworkExpressionData>
        network_expressions;
  } noncompensated;

  struct Compensated
  {
    score::hash_map<
        Path<Scenario::TimeSyncModel>,
        std::function<void(Id<Client>, qint64)>>
        trigger_triggered;
  } compensated;

  void on_stop();

  void sessionChanged() W_SIGNAL(sessionChanged);


  const ObjectMetadata* get_metadata(const Scenario::IntervalModel& obj) const noexcept;
  const ObjectMetadata* get_metadata(const Scenario::EventModel& obj) const noexcept;
  const ObjectMetadata* get_metadata(const Scenario::TimeSyncModel& obj) const noexcept;
  const ObjectMetadata* get_metadata(const Process::ProcessModel& obj) const noexcept;
  ObjectMetadata* get_metadata(const Scenario::IntervalModel& obj) noexcept;
  ObjectMetadata* get_metadata(const Scenario::EventModel& obj) noexcept;
  ObjectMetadata* get_metadata(const Scenario::TimeSyncModel& obj) noexcept;
  ObjectMetadata* get_metadata(const Process::ProcessModel& obj) noexcept;
  void set_metadata(const Scenario::IntervalModel& obj, const ObjectMetadata& m);
  void set_metadata(const Scenario::EventModel& obj, const ObjectMetadata& m);
  void set_metadata(const Scenario::TimeSyncModel& obj, const ObjectMetadata& m);
  void set_metadata(const Process::ProcessModel& obj, const ObjectMetadata& m);
  void unset_metadata(const Scenario::IntervalModel& obj);
  void unset_metadata(const Scenario::EventModel& obj);
  void unset_metadata(const Scenario::TimeSyncModel& obj);
  void unset_metadata(const Process::ProcessModel& obj);


 const std::unordered_map<const Scenario::IntervalModel*, ObjectMetadata>& intervalMetadatas() const noexcept;
 const std::unordered_map<const Scenario::EventModel*, ObjectMetadata>& eventMetadatas() const noexcept;
 const std::unordered_map<const Scenario::TimeSyncModel*, ObjectMetadata>& syncMetadatas() const noexcept;
 const std::unordered_map<const Process::ProcessModel*, ObjectMetadata>& processMetadatas() const noexcept;
 // std::unordered_map<const Scenario::IntervalModel*, ObjectMetadata>& intervalMetadatas() noexcept
 // { return m_intervalsGroups; }
 // std::unordered_map<const Scenario::EventModel*, ObjectMetadata>& eventMetadatas() noexcept
 // { return m_eventGroups; }
 // std::unordered_map<const Scenario::TimeSyncModel*, ObjectMetadata>& syncMetadatas() noexcept
 // { return m_syncGroups; }
 // std::unordered_map<const Process::ProcessModel*, ObjectMetadata>& processMetadatas() noexcept
 // { return m_processGroups; }
private:
  EditionPolicy* m_policy{};
  ExecutionPolicy* m_exec{};
  GroupManager* m_groups{};

  std::unordered_map<const Scenario::IntervalModel*, ObjectMetadata> m_intervalsGroups;
  std::unordered_map<const Scenario::EventModel*, ObjectMetadata> m_eventGroups;
  std::unordered_map<const Scenario::TimeSyncModel*, ObjectMetadata> m_syncGroups;
  std::unordered_map<const Process::ProcessModel*, ObjectMetadata> m_processGroups;
};
}
