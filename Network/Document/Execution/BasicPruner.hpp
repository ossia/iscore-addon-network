#pragma once
#include <iscore/model/Identifier.hpp>
namespace Engine
{
namespace Execution
{
class ConstraintComponent;
class TimeNodeComponent;
class Context;
class ProcessComponent;
}
}
namespace Scenario
{
class ScenarioInterface;
}
namespace Network
{
class Group;
class Client;
class NetworkDocumentPlugin;


enum SyncMode { AsyncOrdered, SyncOrdered, AsyncUnordered, SyncUnordered };

template<typename T, typename Obj>
optional<T> get_metadata(Obj& obj, const QString& s)
{
  auto& m = obj.metadata().getExtendedMetadata();
  auto it = m.constFind(s);
  if(it != m.constEnd())
  {
    const QVariant& var = *it;
    if(var.canConvert<T>())
      return var.value<T>();
  }
  return {};
}
template<typename T>
SyncMode getInfos(const T& obj)
{
  auto syncmode = get_metadata<QString>(obj, "syncmode");
  if(!syncmode || syncmode->isEmpty())
    syncmode = QString("async");
  auto order = get_metadata<QString>(obj, "order");
  if(!order || order->isEmpty())
    order = QString("true");

  const QString async = QStringLiteral("async");
  const QString sync = QStringLiteral("async");
  const QString ordered = QStringLiteral("true");
  const QString unordered = QStringLiteral("false");

  if(syncmode == async && order == ordered)
    return AsyncOrdered;
  else if(syncmode == async && order == unordered)
    return AsyncUnordered;
  else if(syncmode == sync && order == ordered)
    return SyncOrdered;
  else if(syncmode == sync && order == unordered)
    return SyncUnordered;

  return AsyncUnordered;
}
struct NetworkExpressionData
{
  Engine::Execution::TimeNodeComponent& component;
  iscore::hash_map<Id<Client>, optional<bool>> values;
  // Trigger : they all have to be set, and true
  // Event : when they are all set, the truth value of each is taken.

  // Expression observation has to be done on the network.
  // Saved in the network components ? For now in the document plugin?

};

struct BasicPruner
{
  BasicPruner(NetworkDocumentPlugin& d);

  void recurse(Engine::Execution::ConstraintComponent& cst, const Group& cur);
  void recurse(Scenario::ScenarioInterface&, const Group& cur);
  void recurse(Engine::Execution::TimeNodeComponent&);
  void operator()(const Engine::Execution::Context& exec_ctx);

private:
  NetworkDocumentPlugin& doc;
  const Id<Client>& self;

};

struct SharedScenarioPolicy
{
  NetworkDocumentPlugin& doc;
  const Id<Client>& self;

  void operator()(
      Engine::Execution::ProcessComponent& comp,
      Scenario::ScenarioInterface& ip,
      const Group& cur);

  void operator()(Engine::Execution::ConstraintComponent& cst, const Group& cur);

  //! Todo isn't this the code for the mixed mode actually ?
  //! In the "shared" mode we could assume that evaluation entering / leaving is the same
  //! for everyone...
  void operator()(Engine::Execution::TimeNodeComponent& comp, const Group& parent_group);
};
}
