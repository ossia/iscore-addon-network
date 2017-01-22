#include "FreeScenarioPolicy.hpp"
#include <Engine/Executor/ProcessComponent.hpp>

namespace Network
{

void FreeScenarioPolicy::operator()(
    Engine::Execution::ProcessComponent& comp,
    Scenario::ScenarioInterface& ip,
    const Group& cur)
{
  // if on the group enable everything, else disable everything (maybe even remove it from the executor)
  comp.OSSIAProcess().enable(cur.hasClient(ctx.self));
}

}
