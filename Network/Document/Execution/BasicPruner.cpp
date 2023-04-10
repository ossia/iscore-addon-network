#include "BasicPruner.hpp"

#include <Process/ExecutionContext.hpp>

#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/TimeSync/TimeSyncModel.hpp>
#include <Scenario/Process/ScenarioInterface.hpp>

#include <Execution/BaseScenarioComponent.hpp>
#include <Execution/DocumentPlugin.hpp>

#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Document/Execution/SharedScenarioPolicy.hpp>
#include <Network/Group/Group.hpp>
#include <Network/Group/GroupManager.hpp>
#include <Network/Session/Session.hpp>
namespace Network
{
BasicPruner::BasicPruner(NetworkDocumentPlugin& d)
    : ctx{
        d, *d.policy().session(), d.groupManager(),
        d.policy().session()->localClient().id(), d.policy().session()->master().id()}
{
}

void BasicPruner::recurse(Execution::IntervalComponent& cst, const Group& cur)
{
  ctx.doc.noncompensated.trigger_evaluation_entered.clear();
  ctx.doc.noncompensated.trigger_evaluation_finished.clear();
  ctx.doc.noncompensated.trigger_triggered.clear();
  ctx.doc.noncompensated.interval_speed_changed.clear();
  ctx.doc.noncompensated.network_expressions.clear();

  ctx.doc.compensated.trigger_triggered.clear();
  SharedScenarioPolicy{ctx}(cst, cur);
}

void BasicPruner::recurse(Scenario::ScenarioInterface& ip, const Group& cur) { }

void BasicPruner::recurse(Execution::TimeSyncComponent& comp)
{
  // Check the previous / next intervals.
  // If some happen to have to execute in different computers
  // i.e. for all intervals, list their group
  // for these groups, list their clients.
  // if there are at least two clients we have to insert a synchronization
  // point.

  // Question : split in two ticks or do it in a single tick ?
  // Answer : The i-score engine already does this.

  // Give a "group" to the timenode : all the machines of the group have to
  // verify the condition for it to become true.
  // The machines not in this group don't have the expression, they just wait
  // to be started.

  // Who keeps track of the consensus for each expression ?
  // For now, elect a "group leader" ? Or just use the master for this ? Choose
  // a leader for each trigger ? This means that the Client and the Master
  // algorithm is different.

  // Same for event.

  // Algorithm:
  // If the computer executes this trigger.
  // When the sub-expression becomes true, a message is sent to the master
  // When the master gets answers for all clients, he computes the execution
  // date and sends it to them (and to all computer that is not in "free" mode
  // for this trigger ?)

  // What if two computers execute a scenario in "parallel" mode ?
  // Execution modes for processes / intervals : parallel / synchronized / only
  // one ?
}

void BasicPruner::operator()(
    const Execution::Context& exec_ctx, const Execution::BaseScenarioElement& scenar)
{
  // We mute all the processes that are not in a group
  // of this client.
  auto& root = scenar.baseInterval();

  // Let's assume for now that we start in the "all" group...
  recurse(root, *ctx.gm.group(ctx.gm.defaultGroup()));
}
}
