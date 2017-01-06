#include "BasicPruner.hpp"
#include <Network/Group/Group.hpp>
#include <Network/Group/GroupManager.hpp>
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Session/Session.hpp>
#include <Network/Document/Execution/DateExpression.hpp>

#include <Engine/Executor/BaseScenarioComponent.hpp>
#include <Engine/Executor/ConstraintComponent.hpp>
#include <Engine/Executor/TimeNodeComponent.hpp>
#include <Scenario/Process/ScenarioInterface.hpp>
#include <Scenario/Document/Constraint/ConstraintModel.hpp>
#include <Scenario/Document/TimeNode/TimeNodeModel.hpp>

#include <ossia/editor/scenario/time_node.hpp>
namespace Network
{

BasicPruner::BasicPruner(NetworkDocumentPlugin& d)
  : doc{d}
  , self{doc.policy()->session()->localClient().id()}
{

}

void BasicPruner::recurse(Engine::Execution::ConstraintComponent& cst, const Group& cur)
{
  const auto& gm = doc.groupManager();
  // First look if there is a group

  //auto comp = iscore::findComponent<GroupMetadata>(cst.iscoreConstraint().components());
  //if(comp)
  {

  }
  //else
  {
    // We assume that we keep the parent group.
  }

  Scenario::ConstraintModel& constraint = cst.iscoreConstraint();

  // If no group found through components, maybe through metadata :
  const QVariantMap& m = constraint.metadata().getExtendedMetadata();

  // Default case :
  const Group* cur_group = &cur;

  auto it = m.find("group");
  if(it != m.end())
  {
    auto str = it->toString();
    if(str == "all")
    {
      cur_group = gm.group(gm.defaultGroup());
    }
    else if(str == "parent" || str.isEmpty())
    {
      // Default
    }
    else
    {
      // look for a group of this name
      auto group = gm.findGroup(str);
      if(group)
      {
        cur_group = group; // Else we default to the "parent" case.
      }
    }
  }

  ISCORE_ASSERT(cur_group);

  bool isMuted = !cur_group->hasClient(self);
  // Mute the processes that are not meant to execute there.
  constraint.setExecutionState(isMuted
                                 ? Scenario::ConstraintExecutionState::Muted
                                 : Scenario::ConstraintExecutionState::Enabled);

  for(const auto& process : cst.processes())
  {
    auto& proc = process->OSSIAProcess();
    proc.mute(isMuted);
  }

  // Recursion
  for(const auto& process : cst.processes())
  {
    auto ip = dynamic_cast<Scenario::ScenarioInterface*>(&process->process());
    if(ip)
    {
      recurse(*ip, *cur_group);
    }
  }

}

void BasicPruner::recurse(Scenario::ScenarioInterface& ip, const Group& cur)
{

  for(Scenario::TimeNodeModel& tn : ip.getTimeNodes())
  {
    auto comp = iscore::findComponent<Engine::Execution::TimeNodeComponent>(tn.components());
    if(comp)
    {
      recurse(*comp);

    }
  }

  for(Scenario::ConstraintModel& cst : ip.getConstraints())
  {
    auto comp = iscore::findComponent<Engine::Execution::ConstraintComponent>(cst.components());
    if(comp)
      recurse(*comp, cur);
  }

}

void BasicPruner::recurse(Engine::Execution::TimeNodeComponent& comp)
{
  // Check the previous / next constraints.
  // If some happen to have to execute in different computers
  // i.e. for all constraints, list their group
  // for these groups, list their clients.
  // if there are at least two clients we have to insert a synchronization point.

  // Question : split in two ticks or do it in a single tick ?
  // Answer : The i-score engine already does this.

  // Give a "group" to the timenode : all the machines of the group have to
  // verify the condition for it to become true.

  // Who keeps track of the consensus for each expression ?

  // Same for event.
  auto expr = std::make_unique<DateExpression>(
                     std::chrono::nanoseconds{std::numeric_limits<int64_t>::max()},
                     comp.makeTrigger());

  ossia::expressions::expression_generic genexp;
  genexp.expr = std::move(expr);
  genexp.add_callback([] (bool b) {
    if(b) {
      // The expression became true, we have to notify others.
    }
  });

  // TODO also do a callback if the max is reached ?
  comp.OSSIATimeNode()->setExpression(std::make_unique<ossia::expression>(std::move(genexp)));
}

void BasicPruner::operator()(const Engine::Execution::Context& exec_ctx)
{
  // We mute all the processes that are not in a group
  // of this client.
  auto& root = exec_ctx.sys.baseScenario()->baseConstraint();

  // Let's assume for now that we start in the "all" group...
  const auto& gm = doc.groupManager();
  recurse(root, *gm.group(gm.defaultGroup()));
}


}
