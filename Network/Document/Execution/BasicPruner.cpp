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



struct FreeScenarioPolicy
{
  NetworkDocumentPlugin& doc;
  const Id<Client>& self;

  void operator()(
      Engine::Execution::ProcessComponent& comp,
      Scenario::ScenarioInterface& ip,
      const Group& cur)
  {
    // if on the group enable everything, else disable everything (maybe even remove it from the executor)
    comp.OSSIAProcess().enable(cur.hasClient(self));
  }
};

template<typename T>
Group& getGroup(GroupManager& gm, Group& cur, const T& obj)
{
  Group* cur_group = &cur;

  /*
  // First look if there is a group
  auto comp = iscore::findComponent<GroupMetadata>(cst.iscoreConstraint().components());
  if(comp)
  {

  }
  else
  {
    // We assume that we keep the parent group.
  }

  // If no group found through components, maybe through metadata :
  */
  auto ostr = get_metadata<QString>(obj, "group");
  if(!ostr)
    return;

  auto& str = *ostr;
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
    auto group = gm.findGroup(ostr);
    if(group)
    {
      cur_group = group; // Else we default to the "parent" case.
    }
  }
  ISCORE_ASSERT(cur_group);
  return *cur_group;
}

struct SharedScenarioPolicy
{
  NetworkDocumentPlugin& doc;
  const Id<Client>& self;

  void operator()(
      Engine::Execution::ProcessComponent& comp,
      Scenario::ScenarioInterface& ip,
      const Group& cur)
  {
    // take the code of BasicPruner

    for(Scenario::TimeNodeModel& tn : ip.getTimeNodes())
    {
      auto comp = iscore::findComponent<Engine::Execution::TimeNodeComponent>(tn.components());
      if(comp)
      {
        operator()(*comp, cur);

      }
    }

    for(Scenario::ConstraintModel& cst : ip.getConstraints())
    {
      auto comp = iscore::findComponent<Engine::Execution::ConstraintComponent>(cst.components());
      if(comp)
        operator()(*comp, cur);
    }

  }

  void operator()(Engine::Execution::ConstraintComponent& cst, const Group& cur)
  {
    const auto& gm = doc.groupManager();
    Scenario::ConstraintModel& constraint = cst.iscoreConstraint();

    const Group& cur_group = getGroup(gm, cur, constraint);

    bool isMuted = !cur_group.hasClient(self);
    // Mute the processes that are not meant to execute there.
    constraint.setExecutionState(isMuted
                                   ? Scenario::ConstraintExecutionState::Muted
                                   : Scenario::ConstraintExecutionState::Enabled);

    for(const auto& process : cst.processes())
    {
      auto& proc = process.second->OSSIAProcess();
      proc.mute(isMuted);
    }

    // Recursion
    for(const auto& process : cst.processes())
    {
      auto ip = dynamic_cast<Scenario::ScenarioInterface*>(&process.second->process());
      if(ip)
      {

        auto syncmode = get_metadata<QString>(process.second->process(), "syncmode");
        if(!syncmode || syncmode->isEmpty())
          syncmode = get_metadata<QString>(constraint, "syncmode");
        if(!syncmode || syncmode->isEmpty())
          syncmode = "shared";

        if(*syncmode == "shared")
        {
          FreeScenarioPolicy{doc, self}(*process.second, *ip, cur_group);
        }
        else if(*syncmode == "mixed")
        {
          MixedScenarioPolicy{doc, self}(*process.second, *ip, cur_group);
        }
        else if(*syncmode == "free")
        {
          SharedScenarioPolicy{doc, self}(*process.second, *ip, cur_group);
        }
      }
    }

  }

  void operator()(Engine::Execution::TimeNodeComponent& comp, const Group& cur)
  {

    // First fetch the required variables.
    auto group = get_metadata<QString>(comp.iscoreTimeNode(), "group");
    if(!group || group->isEmpty())
      group = QString("parent");
    auto syncmode = get_metadata<QString>(comp.iscoreTimeNode(), "syncmode");
    if(!syncmode || syncmode->isEmpty())
      syncmode = QString("async");
    auto order = get_metadata<QString>(comp.iscoreTimeNode(), "order");
    if(!order || order->isEmpty())
      order = QString("true");

    // If this group has this expression
    if()


    // If the computer does not execute this trigger.
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
};

struct MixedScenarioPolicy
{
  NetworkDocumentPlugin& doc;
  const Id<Client>& self;

  void operator()(
      Engine::Execution::ProcessComponent& comp,
      Scenario::ScenarioInterface& ip,
      const Group& cur)
  {
    // muzukashi

  }
};


BasicPruner::BasicPruner(NetworkDocumentPlugin& d)
  : doc{d}
  , self{doc.policy().session()->localClient().id()}
{

}

template<typename T, typename Obj>
optional<T> get_metadata(Obj& obj, const QString& s)
{
  auto& m = obj.metadata().getExtendedMetadata();
  auto it = m.constFind(s);
  if(it != m.constEnd())
  {
    const QVariant& var = *it;
    if(var.canConvert<T>())
      return var.value<T>();
  }
  return {};
}
void BasicPruner::recurse(Engine::Execution::ConstraintComponent& cst, const Group& cur)
{

}

void BasicPruner::recurse(Scenario::ScenarioInterface& ip, const Group& cur)
{

}

struct NetworkExpressionData
{
  Engine::Execution::TimeNodeComponent& component;
  iscore::hash_map<Id<Client>, optional<bool>> values;
  // Trigger : they all have to be set, and true
  // Event : when they are all set, the truth value of each is taken.

  // Expression observation has to be done on the network.
  // Saved in the network components ? For now in the document plugin?

};
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
  // The machines not in this group don't have the expression, they just wait to be started.

  // Who keeps track of the consensus for each expression ?
  // For now, elect a "group leader" ? Or just use the master for this ? Choose a leader for each trigger ?
  // This means that the Client and the Master algorithm is different.

  // Same for event.

  // Algorithm:
  // If the computer executes this trigger.
  // When the sub-expression becomes true, a message is sent to the master
  // When the master gets answers for all clients, he computes the execution date and
  // sends it to them (and to all computer that is not in "free" mode for this trigger ?)

  // What if two computers execute a scenario in "parallel" mode ?
  // Execution modes for processes / constraints : parallel / synchronized / only one ?

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
