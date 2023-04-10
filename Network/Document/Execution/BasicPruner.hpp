#pragma once
#include <score/model/Identifier.hpp>
#include <score/tools/std/HashMap.hpp>

#include <Network/Document/Execution/Context.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <Network/Group/GroupManager.hpp>
#include <Network/Session/Session.hpp>
namespace Scenario
{
class ScenarioInterface;
}
namespace Execution
{
class BaseScenarioElement;
}
namespace Network
{
struct SCORE_ADDON_NETWORK_EXPORT BasicPruner
{
  BasicPruner(NetworkDocumentPlugin& d);

  void recurse(Execution::IntervalComponent& cst, const Group& cur);
  void recurse(Scenario::ScenarioInterface&, const Group& cur);
  void recurse(Execution::TimeSyncComponent&);
  void operator()(
      const Execution::Context& exec_ctx, const Execution::BaseScenarioElement& scenar);

private:
  NetworkPrunerContext ctx;
};
}
