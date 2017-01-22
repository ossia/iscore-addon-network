#include "SharedScenarioPolicy.hpp"
#include <Network/Session/Session.hpp>
#include <Network/Document/Execution/DateExpression.hpp>
#include <Network/Document/Execution/MixedScenarioPolicy.hpp>
#include <Network/Document/Execution/FreeScenarioPolicy.hpp>
#include <Scenario/Document/TimeNode/Trigger/TriggerModel.hpp>
#include <Engine/Executor/TimeNodeComponent.hpp>
#include <Engine/Executor/ConstraintComponent.hpp>
#include <Scenario/Process/ScenarioModel.hpp>
#include <iscore/model/path/PathSerialization.hpp>
#include <ossia/editor/scenario/time_node.hpp>

namespace Network
{

struct AsyncUnorderedInGroup
{
  void operator()(
      NetworkPrunerContext& ctx,
      Engine::Execution::TimeNodeComponent& comp,
      const Path<Scenario::TimeNodeModel>& path)
  {
    auto base_expr = std::make_shared<expression_with_callback>(comp.makeTrigger().release());

    // Common case : set the expression
    auto expr = std::make_unique<AsyncExpression>();
    auto expr_ptr = expr.get();

    ossia::expressions::expression_generic genexp;
    genexp.expr = std::move(expr);

    comp.OSSIATimeNode()->setExpression(std::make_unique<ossia::expression>(std::move(genexp)));

    // Then set specific callbacks for outside events
    auto& session = ctx.session;
    auto master = ctx.master;
    ctx.doc.trigger_evaluation_entered.emplace(path, [=,&session] (Id<Client> orig) {
      base_expr->it = ossia::expressions::add_callback(
                        *base_expr->expr,
                        [=,&session] (bool b) {
        if(b)
        {
          session.emitMessage(master, session.makeMessage("/trigger_expression_true", path));
        }
      });
    });

    ctx.doc.trigger_evaluation_finished.emplace(path, [=] (Id<Client> orig, bool b) {
      if(base_expr->it)
        ossia::expressions::remove_callback(
              *base_expr->expr, *base_expr->it);

      expr_ptr->ping(); // TODO how to transmit the max bound information ??
    });

    ctx.doc.trigger_triggered.emplace(path, [=] (Id<Client> orig) {
      if(base_expr->it)
        ossia::expressions::remove_callback(
              *base_expr->expr, *base_expr->it);

      expr_ptr->ping();
    });

  }
};


struct AsyncUnorderedOutOfGroup
{
  void operator()(
      NetworkPrunerContext& ctx,
      Engine::Execution::TimeNodeComponent& comp,
      const Path<Scenario::TimeNodeModel>& path)
  {
    auto expr = std::make_unique<AsyncExpression>();
    auto expr_ptr = expr.get();

    ctx.doc.trigger_triggered.emplace(path, [=] (Id<Client> orig) {
      expr_ptr->ping();
    });
    ctx.doc.trigger_evaluation_finished.emplace(path, [=] (Id<Client> orig, bool) {
      expr_ptr->ping(); // TODO how to transmit the max bound information ??
    });

    ossia::expressions::expression_generic genexp;
    genexp.expr = std::move(expr);

    comp.OSSIATimeNode()->setExpression(std::make_unique<ossia::expression>(std::move(genexp)));
  }
};



void SharedScenarioPolicy::operator()(
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

void SharedScenarioPolicy::operator()(
    Engine::Execution::ConstraintComponent& cst,
    const Group& cur)
{
  Scenario::ConstraintModel& constraint = cst.iscoreConstraint();

  const Group& cur_group = getGroup(ctx.gm, cur, constraint);

  bool isMuted = !cur_group.hasClient(ctx.self);
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
        syncmode = QStringLiteral("shared");

      if(*syncmode == "shared")
      {
        SharedScenarioPolicy{ctx}(*process.second, *ip, cur_group);
      }
      else if(*syncmode == "mixed")
      {
        MixedScenarioPolicy{ctx}(*process.second, *ip, cur_group);
      }
      else if(*syncmode == "free")
      {
        FreeScenarioPolicy{ctx}(*process.second, *ip, cur_group);
      }
    }
  }
}


void SharedScenarioPolicy::operator()(
    Engine::Execution::TimeNodeComponent& comp,
    const Group& parent_group)
{
  // First fetch the required variables.
  const Group& tn_group = getGroup(ctx.gm, parent_group, comp.iscoreTimeNode());

  auto sync = getInfos(comp.iscoreTimeNode());
  Path<Scenario::TimeNodeModel> path{comp.iscoreTimeNode()};

  if(comp.iscoreTimeNode().trigger()->active())
  {
    auto& session = ctx.session;
    auto master = ctx.master;
    // Each trigger sends its own data, the master will choose the relevant info
    comp.OSSIATimeNode()->enteredEvaluation.add_callback([=,&session,&master] {
      session.emitMessage(master, session.makeMessage("/trigger_entered", path));
    });
    comp.OSSIATimeNode()->leftEvaluation.add_callback([=,&session] {
      session.emitMessage(master, session.makeMessage("/trigger_left", path));
    });
    comp.OSSIATimeNode()->finishedEvaluation.add_callback([=,&session] (bool b) {
      // b : max bound reached
      session.emitMessage(master, session.makeMessage("/trigger_finished", path, b));
    });
    comp.OSSIATimeNode()->triggered.add_callback([=,&session] {
      session.emitMessage(master, session.makeMessage("/triggered", path));
    });

    // If this group has this expression
    // Since we're in the SharedPolicy, everybody will get the same information
    if(tn_group.hasClient(ctx.self))
    {
      // We will actually evaluate the expression.

      switch(sync)
      {
        case SyncMode::AsyncOrdered:
          break;
        case SyncMode::AsyncUnordered:
        {
          AsyncUnorderedInGroup{}(ctx, comp, path);
          break;
        }
        case SyncMode::SyncOrdered:
          break;
        case SyncMode::SyncUnordered:
          break;
      }
    }
    else
    {
      // Not in the group : we wait.
      switch(sync)
      {
        case SyncMode::AsyncOrdered:
          break;
        case SyncMode::AsyncUnordered:
        {
          AsyncUnorderedOutOfGroup{}(ctx, comp, path);
          break;
        }
        case SyncMode::SyncOrdered:
          break;
        case SyncMode::SyncUnordered:
          break;
      }

    }
  }
  else
  {
    // Trigger not active. For now let's just hope that everything happens correctly.
  }
  /*
    auto expr = std::make_unique<DateExpression>(
          std::chrono::nanoseconds{std::numeric_limits<int64_t>::max()},
          comp.makeTrigger());*/
}
}
