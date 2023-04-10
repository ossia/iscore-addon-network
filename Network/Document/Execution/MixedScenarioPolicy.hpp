#pragma once
#include <Network/Document/Execution/Context.hpp>
namespace Scenario
{
class ScenarioInterface;
}
namespace Network
{

struct MixedScenarioPolicy
{
  NetworkPrunerContext& ctx;

  void operator()(
      Execution::ProcessComponent& comp, Scenario::ScenarioInterface& ip,
      const Group& cur);

  void operator()(Execution::TimeSyncComponent& comp, const Group& parent_group);
};
}
