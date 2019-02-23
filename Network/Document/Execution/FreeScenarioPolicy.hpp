#pragma once
#include <Network/Document/Execution/Context.hpp>
namespace Scenario
{
class ScenarioInterface;
}

namespace Network
{

struct FreeScenarioPolicy
{
  NetworkPrunerContext& ctx;

  void operator()(
      Execution::ProcessComponent& comp,
      Scenario::ScenarioInterface& ip,
      const Group& cur);
};
}
