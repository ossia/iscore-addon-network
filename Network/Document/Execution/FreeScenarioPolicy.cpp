#include "FreeScenarioPolicy.hpp"

#include <Process/Execution/ProcessComponent.hpp>

#include <Scenario/Process/ScenarioExecution.hpp>
#include <Scenario/Process/ScenarioModel.hpp>

#include <score/model/ComponentUtils.hpp>

namespace Network
{
void FreeScenarioPolicy::operator()(
    Execution::ProcessComponent& comp, Scenario::ScenarioInterface& ip, const Group& cur)
{
  // if on the group enable everything, else disable everything (maybe even
  // remove it from the executor)
  const bool runs = cur.hasClient(ctx.self);
  comp.OSSIAProcess().enable(runs);

  auto set_flag = [runs](auto& obj) {
    auto flags = obj.networkFlags();
    if(runs)
      flags = flags | Process::NetworkFlags::Active;
    else
      flags = (Process::NetworkFlags)(flags & ~Process::NetworkFlags::Active);
    obj.setNetworkFlags(flags);
  };
  set_flag(comp.process());

  for(auto& obj : ip.getIntervals())
  {
    set_flag(obj);
    for(auto& proc : obj.processes)
    {
      set_flag(proc);
      if(auto* scenar = qobject_cast<Scenario::ProcessModel*>(&proc))
      {
        auto comp
            = score::findComponent<Execution::ScenarioComponent>(scenar->components());
        (*this)(*comp, *scenar, cur);
      }
    }
  }
  for(auto& obj : ip.getTimeSyncs())
  {
    set_flag(obj);
  }
  for(auto& obj : ip.getEvents())
  {
    set_flag(obj);
  }
}
}
