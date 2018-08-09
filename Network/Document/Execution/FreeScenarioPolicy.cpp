#include "FreeScenarioPolicy.hpp"
#include <Process/Execution/ProcessComponent.hpp>

namespace Network
{
void FreeScenarioPolicy::operator()(
    Execution::ProcessComponent& comp,
    Scenario::ScenarioInterface& ip,
    const Group& cur)
{
  // if on the group enable everything, else disable everything (maybe even remove it from the executor)
  comp.OSSIAProcess().enable(cur.hasClient(ctx.self));
}

}
