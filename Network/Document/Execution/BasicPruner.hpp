#pragma once
#include <Network/Group/GroupManager.hpp>
#include <Network/Session/Session.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <Network/Document/Execution/Context.hpp>
#include <iscore/model/Identifier.hpp>
#include <iscore/tools/std/HashMap.hpp>
namespace Scenario
{
class ScenarioInterface;
}
namespace Network
{
struct ISCORE_ADDON_NETWORK_EXPORT BasicPruner
{
  BasicPruner(NetworkDocumentPlugin& d);

  void recurse(Engine::Execution::ConstraintComponent& cst, const Group& cur);
  void recurse(Scenario::ScenarioInterface&, const Group& cur);
  void recurse(Engine::Execution::TimeSyncComponent&);
  void operator()(const Engine::Execution::Context& exec_ctx);

private:
  NetworkPrunerContext ctx;

};

}
