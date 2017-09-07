#pragma once
#include <Network/Document/Execution/Context.hpp>
namespace Scenario { class ScenarioInterface; }
namespace Network
{

struct SharedScenarioPolicy
{
  NetworkPrunerContext& ctx;

  void operator()(
      Engine::Execution::ProcessComponent& comp,
      Scenario::ScenarioInterface& ip,
      const Group& cur);

  void operator()(Engine::Execution::IntervalComponent& cst, const Group& cur);
  void operator()(Engine::Execution::EventComponent& cst, const Group& cur);

  //! Todo isn't this the code for the mixed mode actually ?
  //! In the "shared" mode we could assume that evaluation entering / leaving is the same
  //! for everyone...
  void operator()(Engine::Execution::TimeSyncComponent& comp, const Group& parent_group);

  void setupMaster(
      Engine::Execution::TimeSyncComponent& comp,
      Path<Scenario::TimeSyncModel> p,
      const Group& parent_group,
      SyncMode sync);
};

}
