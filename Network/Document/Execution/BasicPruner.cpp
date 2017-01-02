#include "BasicPruner.hpp"
#include <Network/Group/Group.hpp>
#include <Network/Group/GroupManager.hpp>
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Session/Session.hpp>

#include <Engine/Executor/BaseScenarioComponent.hpp>
#include <Engine/Executor/ConstraintComponent.hpp>
#include <Scenario/Process/ScenarioInterface.hpp>
#include <Scenario/Document/Constraint/ConstraintModel.hpp>

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

  // Mute the processes that are not meant to execute there.
  constraint.setExecutionState(Scenario::ConstraintExecutionState::Muted);

  for(const auto& process : cst.processes())
  {
    auto& proc = process->OSSIAProcess();
    proc.mute(!cur_group->hasClient(self));
  }

  // Recursion
  for(const auto& process : cst.processes())
  {
    auto ip = dynamic_cast<Scenario::ScenarioInterface*>(&process->process());
    if(ip)
    {
      for(Scenario::ConstraintModel& cst : ip->getConstraints())
      {
        auto comp = iscore::findComponent<Engine::Execution::ConstraintComponent>(cst.components());
        if(comp)
          recurse(*comp, *cur_group);
      }
    }
  }

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
