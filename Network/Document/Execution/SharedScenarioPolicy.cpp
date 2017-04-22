#include "SharedScenarioPolicy.hpp"
#include <Network/Session/Session.hpp>
#include <Network/Document/Execution/DateExpression.hpp>
#include <Network/Document/Execution/MixedScenarioPolicy.hpp>
#include <Network/Document/Execution/FreeScenarioPolicy.hpp>
#include <Scenario/Document/TimeNode/Trigger/TriggerModel.hpp>
#include <Engine/Executor/TimeNodeComponent.hpp>
#include <Engine/Executor/ConstraintComponent.hpp>
#include <Scenario/Process/ScenarioModel.hpp>
#include <Scenario/Process/Algorithms/Accessors.hpp>
#include <iscore/model/path/PathSerialization.hpp>
#include <ossia/editor/scenario/time_node.hpp>

namespace Network
{

struct ExpressionAsyncInGroup
{
  struct ExprData {
    std::shared_ptr<expression_with_callback> shared_expr;
    AsyncExpression* async_expr{};
  };

  ExprData setupExpr(
      Engine::Execution::TimeNodeComponent& comp)
  {
    // Wrap the expresion
    ExprData e;
    e.shared_expr = std::make_shared<expression_with_callback>(comp.makeTrigger().release());
    e.async_expr = new AsyncExpression;

    comp.OSSIATimeNode()->set_expression(
          std::make_unique<ossia::expression>(
            ossia::expressions::expression_generic{
              std::unique_ptr<ossia::expressions::expression_generic_base>(e.async_expr)})
          );

    return e;
  }
};

struct SharedAsyncUnorderedInGroup : public ExpressionAsyncInGroup
{
  void operator()(
      NetworkPrunerContext& ctx,
      Engine::Execution::TimeNodeComponent& comp,
      const Path<Scenario::TimeNodeModel>& path)
  {
    ExprData e = setupExpr(comp);

    // Then set specific callbacks for outside events
    auto& session = ctx.session;
    auto master = ctx.master;
    auto& mapi = ctx.mapi;

    // When the trigger enters evaluation
    ctx.doc.trigger_evaluation_entered.emplace(path, [=,&session,&mapi] (Id<Client> orig) {
      e.shared_expr->it = ossia::expressions::add_callback(
                            *e.shared_expr->expr,
                            [=,&session,&mapi] (bool b) {
        if(b)
        {
          session.emitMessage(
                master,
                session.makeMessage(mapi.trigger_expression_true, path));
        }
      });
    });


    // When the trigger finishes evaluation
    ctx.doc.trigger_evaluation_finished.emplace(path, [=] (Id<Client> orig, bool b) {
      if(e.shared_expr->it)
        ossia::expressions::remove_callback(*e.shared_expr->expr, *e.shared_expr->it);

      e.async_expr->ping(); // TODO how to transmit the max bound information ??
    });

    // When the trigger can be triggered
    ctx.doc.trigger_triggered.emplace(path, [=] (Id<Client> orig) {
      if(e.shared_expr->it)
        ossia::expressions::remove_callback(
              *e.shared_expr->expr, *e.shared_expr->it);

      e.async_expr->ping();
    });
  }
};


struct SharedAsyncOrderedInGroup : public ExpressionAsyncInGroup
{
  void operator()(
      NetworkPrunerContext& ctx,
      Engine::Execution::TimeNodeComponent& comp,
      const Path<Scenario::TimeNodeModel>& path)
  {
    ExprData e = setupExpr(comp);

    // Then set specific callbacks for outside events
    auto& session = ctx.session;
    auto& mapi = ctx.mapi;
    auto master = ctx.master;

    // When the trigger enters evaluation
    ctx.doc.trigger_evaluation_entered.emplace(path, [=,&session,&mapi] (Id<Client> orig) {
      e.shared_expr->it = ossia::expressions::add_callback(
                        *e.shared_expr->expr,
                        [=,&session] (bool b) {
        if(b)
        {
          session.emitMessage(
                master,
                session.makeMessage(mapi.trigger_expression_true, path));
        }
      });
    });

    // When the trigger finishes evaluation
    ctx.doc.trigger_evaluation_finished.emplace(path, [=,&session] (Id<Client> orig, bool b) {
      if(e.shared_expr->it)
        ossia::expressions::remove_callback(*e.shared_expr->expr, *e.shared_expr->it);

      e.async_expr->ping(); // TODO how to transmit the max bound information ??

      // Since we're ordered, we inform the master when we're ready to trigger the followers
      session.emitMessage(master, session.makeMessage(mapi.trigger_previous_completed, path));
    });

    // When the trigger can be triggered
    ctx.doc.trigger_triggered.emplace(path, [=,&session] (Id<Client> orig) {
      if(e.shared_expr->it)
        ossia::expressions::remove_callback(
              *e.shared_expr->expr, *e.shared_expr->it);

      e.async_expr->ping();

      // Since we're ordered, we inform the master when we're ready to trigger the followers
      session.emitMessage(master, session.makeMessage(mapi.trigger_previous_completed, path));
    });

  }
};


struct SharedAsyncUnorderedOutOfGroup
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

    comp.OSSIATimeNode()->set_expression(
          std::make_unique<ossia::expression>(
            ossia::expressions::expression_generic{
              std::move(expr)}));
  }
};


struct SharedAsyncOrderedOutOfGroup
{
  void operator()(
      NetworkPrunerContext& ctx,
      Engine::Execution::TimeNodeComponent& comp,
      const Path<Scenario::TimeNodeModel>& path)
  {
    auto expr = std::make_unique<AsyncExpression>();
    auto expr_ptr = expr.get();
    auto& session = ctx.session;
    auto& mapi = ctx.mapi;
    auto master = ctx.master;

    ctx.doc.trigger_triggered.emplace(path, [=,&session,&mapi] (Id<Client> orig) {
      expr_ptr->ping();
      session.emitMessage(master, session.makeMessage(mapi.trigger_previous_completed, path));
    });
    ctx.doc.trigger_evaluation_finished.emplace(path, [=,&session,&mapi] (Id<Client> orig, bool) {
      expr_ptr->ping(); // TODO how to transmit the max bound information ??
      session.emitMessage(master, session.makeMessage(mapi.trigger_previous_completed, path));
    });

    comp.OSSIATimeNode()->set_expression(
          std::make_unique<ossia::expression>(
            ossia::expressions::expression_generic{
              std::move(expr)}));
  }
};





void SharedScenarioPolicy::operator()(
    Engine::Execution::ProcessComponent& c,
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
  auto& mapi = ctx.mapi;
  // First fetch the required variables.
  const Group& tn_group = getGroup(ctx.gm, parent_group, comp.iscoreTimeNode());

  auto sync = getInfos(comp.iscoreTimeNode());
  Path<Scenario::TimeNodeModel> path{comp.iscoreTimeNode()};

  if(comp.iscoreTimeNode().trigger()->active())
  {
    auto& session = ctx.session;
    auto master = ctx.master;
    // Each trigger sends its own data, the master will choose the relevant info
    comp.OSSIATimeNode()->entered_evaluation.add_callback([path,&mapi,&session,&master] {
      session.emitMessage(master, session.makeMessage(mapi.trigger_entered, path));
    });
    comp.OSSIATimeNode()->left_evaluation.add_callback([=,&session] {
      session.emitMessage(master, session.makeMessage(mapi.trigger_left, path));
    });
    comp.OSSIATimeNode()->finished_evaluation.add_callback([=,&session] (bool b) {
      // b : max bound reached
      session.emitMessage(master, session.makeMessage(mapi.trigger_finished, path, b));
    });
    comp.OSSIATimeNode()->triggered.add_callback([=,&session] {
      session.emitMessage(master, session.makeMessage(mapi.trigger_triggered, path));
    });

    // If this group has this expression
    // Since we're in the SharedPolicy, everybody will get the same information
    if(tn_group.hasClient(ctx.self))
    {
      // We will actually evaluate the expression.

      switch(sync)
      {
        case SyncMode::AsyncOrdered:
          SharedAsyncOrderedInGroup{}(ctx, comp, path);
          break;
        case SyncMode::AsyncUnordered:
          SharedAsyncUnorderedInGroup{}(ctx, comp, path);
          break;
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
          SharedAsyncOrderedOutOfGroup{}(ctx, comp, path);
          break;
        case SyncMode::AsyncUnordered:
          SharedAsyncUnorderedOutOfGroup{}(ctx, comp, path);
          break;
        case SyncMode::SyncOrdered:
          break;
        case SyncMode::SyncUnordered:
          break;
      }
    }

    setupMaster(comp, path, tn_group, sync);
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

void SharedScenarioPolicy::setupMaster(
    Engine::Execution::TimeNodeComponent& comp,
    Path<Scenario::TimeNodeModel> p,
    const Group& tn_group,
    SyncMode sync)
{
  // Handle the "talk to master" case
  if(ctx.master == ctx.self)
  {
    NetworkExpressionData exp{comp};
    exp.thisGroup = tn_group.id();
    exp.sync = sync;
    exp.pol = ExpressionPolicy::OnFirst; // TODO another

    auto scenar = dynamic_cast<Scenario::ScenarioInterface*>(comp.iscoreTimeNode().parent());

    auto constraint_group = [&] (const Id<Scenario::ConstraintModel>& cst_id)
    {
      auto& cst = scenar->constraint(cst_id);
      auto& grp = getGroup(ctx.gm, tn_group, cst);
      return grp.id();
    };

    {
      // Find all the previous ConstraintComponents.
      auto csts = Scenario::previousConstraints(comp.iscoreTimeNode(), *scenar);
      exp.prevGroups.reserve(csts.size());
      ossia::transform(csts, std::back_inserter(exp.prevGroups), constraint_group);
    }

    {
      auto csts = Scenario::nextConstraints(comp.iscoreTimeNode(), *scenar);
      exp.nextGroups.reserve(csts.size());
      ossia::transform(csts, std::back_inserter(exp.nextGroups), constraint_group);
    }

    ctx.doc.network_expressions.emplace(std::move(p), std::move(exp));
  }

}
}
