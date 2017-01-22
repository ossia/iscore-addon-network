#pragma once
#include <Network/Document/Execution/Context.hpp>


namespace Network
{

struct FreeScenarioPolicy
{
  NetworkPrunerContext& ctx;

  void operator()(
      Engine::Execution::ProcessComponent& comp,
      Scenario::ScenarioInterface& ip,
      const Group& cur);
};

}
