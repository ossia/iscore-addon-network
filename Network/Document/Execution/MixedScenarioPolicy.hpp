#pragma once
#include <Network/Document/Execution/Context.hpp>

namespace Network
{

struct MixedScenarioPolicy
{
  NetworkPrunerContext& ctx;

  void operator()(
      Engine::Execution::ProcessComponent& comp,
      Scenario::ScenarioInterface& ip,
      const Group& cur);

  void operator()(Engine::Execution::TimeNodeComponent& comp, const Group& parent_group);
};


}
