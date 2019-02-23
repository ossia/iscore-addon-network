#pragma once
#include <Network/Document/Execution/Context.hpp>
namespace Scenario
{
class ScenarioInterface;
}
namespace Network
{

struct SharedScenarioPolicy
{
  NetworkPrunerContext& ctx;

  void operator()(
      Execution::ProcessComponent& comp,
      Scenario::ScenarioInterface& ip,
      const Group& cur);

  void operator()(Execution::IntervalComponent& cst, const Group& cur);
  void operator()(Execution::EventComponent& cst, const Group& cur);

  //! Todo isn't this the code for the mixed mode actually ?
  //! In the "shared" mode we could assume that evaluation entering / leaving
  //! is the same for everyone...
  void
  operator()(Execution::TimeSyncComponent& comp, const Group& parent_group);

  void setupMaster(
      Execution::TimeSyncComponent& comp,
      Path<Scenario::TimeSyncModel> p,
      const Group& parent_group,
      SyncMode sync);
};
}
