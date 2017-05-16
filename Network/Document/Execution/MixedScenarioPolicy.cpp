#include "MixedScenarioPolicy.hpp"

namespace Network
{

void MixedScenarioPolicy::operator()(Engine::Execution::ProcessComponent& comp, Scenario::ScenarioInterface& ip, const Group& cur)
{
  // muzukashi

}

void MixedScenarioPolicy::operator()(Engine::Execution::TimeNodeComponent& comp, const Group& parent_group)
{/*
    const auto& gm = doc.groupManager();
    // First fetch the required variables.
    const Group& tn_group = getGroup(gm, parent_group, comp.iscoreTimeNode());

    auto sync = getInfos(comp.iscoreTimeNode());
    Path<Scenario::TimeNodeModel> path{comp.iscoreTimeNode()};

    if(comp.iscoreTimeNode().trigger()->active())
    {
      // Each trigger sends its own data, the master will choose the relevant info
      comp.OSSIATimeNode()->enteredEvaluation.add_callback([] {
        // Send message to master
      });
      comp.OSSIATimeNode()->leftEvaluation.add_callback([] {
        // Send message to master
      });
      comp.OSSIATimeNode()->finishedEvaluation.add_callback([] (bool b) {
        // Send message to master

        // b : max bound reached
      });
      comp.OSSIATimeNode()->triggered.add_callback([] {
        // Send message to master
      });

      // If this group has this expression
      // Since we're in the SharedPolicy, everybody will get the same information
      if(tn_group.hasClient(self))
      {
        // We will actually evaluate the expression.
        auto base_expr = std::make_shared<expression_with_callback>(comp.makeTrigger().release());

        switch(sync)
        {
          case SyncMode::AsyncOrdered:
            break;
          case SyncMode::AsyncUnordered:
          {
            // Common case : set the expression
            auto expr = std::make_unique<AsyncExpression>();
            auto expr_ptr = expr.get();

            ossia::expressions::expression_generic genexp;
            genexp.expr = std::move(expr);

            comp.OSSIATimeNode()->setExpression(std::make_unique<ossia::expression>(std::move(genexp)));


            // Then set specific callbacks for outside events
            doc.noncompensated.trigger_evaluation_entered[path] = [=] {
              base_expr->it = ossia::expressions::add_callback(
                    *base_expr->expr,
                    [] (bool b) {
                if(b)
                {
                  // Send message to master

                }
              });
            };

            doc.noncompensated.trigger_evaluation_finished[path] = [=] (bool b) {
              if(base_expr->it)
                ossia::expressions::remove_callback(
                      *base_expr->expr, *base_expr->it);

              expr_ptr->ping(); // TODO how to transmit the max bound information ??
            };

            doc.noncompensated.trigger_triggered[path] = [=] {
              if(base_expr->it)
                ossia::expressions::remove_callback(
                      *base_expr->expr, *base_expr->it);

              expr_ptr->ping();
            };

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
        switch(sync)
        {
          case SyncMode::AsyncOrdered:
            break;
          case SyncMode::AsyncUnordered:
          {
            auto expr = std::make_unique<AsyncExpression>();
            auto expr_ptr = expr.get();

            doc.noncompensated.trigger_triggered[path] = [=] {
              expr_ptr->ping();
            };
            doc.noncompensated.trigger_evaluation_finished[path] = [=] (bool) {
              expr_ptr->ping(); // TODO how to transmit the max bound information ??
            };


            ossia::expressions::expression_generic genexp;
            genexp.expr = std::move(expr);

            comp.OSSIATimeNode()->setExpression(std::make_unique<ossia::expression>(std::move(genexp)));

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
    }*/
  /*
    auto expr = std::make_unique<DateExpression>(
          std::chrono::nanoseconds{std::numeric_limits<int64_t>::max()},
          comp.makeTrigger());*/
}

/*

struct MixedAsyncUnorderedInGroup : public ExpressionAsyncInGroup
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

    // When the trigger enters evaluation
    ctx.doc.noncompensated.trigger_evaluation_entered.emplace(path, [=,&session] (Id<Client> orig) {
      e.shared_expr->it = ossia::expressions::add_callback(
                            *e.shared_expr->expr,
                            [=,&session] (bool b) {
        if(b)
        {
          session.emitMessage(
                master,
                session.makeMessage(ctx.mapi.trigger_expression_true, path));
        }
      });
    });

    // When the trigger finishes evaluation
    ctx.doc.noncompensated.trigger_evaluation_finished.emplace(path, [=] (Id<Client> orig, bool b) {
      if(e.shared_expr->it)
        ossia::expressions::remove_callback(*e.shared_expr->expr, *e.shared_expr->it);

      e.async_expr->ping(); // TODO how to transmit the max bound information ??
    });

    // When the trigger can be triggered
    ctx.doc.noncompensated.trigger_triggered.emplace(path, [=] (Id<Client> orig) {
      if(e.shared_expr->it)
        ossia::expressions::remove_callback(
              *e.shared_expr->expr, *e.shared_expr->it);

      e.async_expr->ping();
    });
  }
};


struct MixedAsyncOrderedInGroup : public ExpressionAsyncInGroup
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

    // When the trigger enters evaluation
    ctx.doc.noncompensated.trigger_evaluation_entered.emplace(path, [=,&session] (Id<Client> orig) {
      e.shared_expr->it = ossia::expressions::add_callback(
                            *e.shared_expr->expr,
                            [=,&session] (bool b) {
        if(b)
        {
          session.emitMessage(
                master,
                session.makeMessage(ctx.mapi.trigger_expression_true, path));
        }
      });
    });

    // When the trigger finishes evaluation
    ctx.doc.noncompensated.trigger_evaluation_finished.emplace(path, [=,&session] (Id<Client> orig, bool b) {
      if(e.shared_expr->it)
        ossia::expressions::remove_callback(*e.shared_expr->expr, *e.shared_expr->it);

      e.async_expr->ping(); // TODO how to transmit the max bound information ??

      // Since we're ordered, we inform the master when we're ready to trigger the followers
      session.emitMessage(master, session.makeMessage(ctx.mapi.trigger_previous_completed, path));
    });

    // When the trigger can be triggered
    ctx.doc.noncompensated.trigger_triggered.emplace(path, [=,&session] (Id<Client> orig) {
      if(e.shared_expr->it)
        ossia::expressions::remove_callback(
              *e.shared_expr->expr, *e.shared_expr->it);

      e.async_expr->ping();

      // Since we're ordered, we inform the master when we're ready to trigger the followers
      session.emitMessage(master, session.makeMessage(ctx.mapi.trigger_previous_completed, path));
    });

  }
};


struct MixedAsyncUnorderedOutOfGroup
{
  void operator()(
      NetworkPrunerContext& ctx,
      Engine::Execution::TimeNodeComponent& comp,
      const Path<Scenario::TimeNodeModel>& path)
  {
    auto expr = std::make_unique<AsyncExpression>();
    auto expr_ptr = expr.get();

    ctx.doc.noncompensated.trigger_triggered.emplace(path, [=] (Id<Client> orig) {
      expr_ptr->ping();
    });
    ctx.doc.noncompensated.trigger_evaluation_finished.emplace(path, [=] (Id<Client> orig, bool) {
      expr_ptr->ping(); // TODO how to transmit the max bound information ??
    });

    comp.OSSIATimeNode()->setExpression(
          std::make_unique<ossia::expression>(
            ossia::expressions::expression_generic{
              std::move(expr)}));
  }
};


struct MixedAsyncOrderedOutOfGroup
{
  void operator()(
      NetworkPrunerContext& ctx,
      Engine::Execution::TimeNodeComponent& comp,
      const Path<Scenario::TimeNodeModel>& path)
  {
    auto expr = std::make_unique<AsyncExpression>();
    auto expr_ptr = expr.get();
    auto& session = ctx.session;
    auto master = ctx.master;

    ctx.doc.noncompensated.trigger_triggered.emplace(path, [=,&session] (Id<Client> orig) {
      expr_ptr->ping();
      session.emitMessage(master, session.makeMessage(ctx.mapi.trigger_previous_completed, path));
    });
    ctx.doc.noncompensated.trigger_evaluation_finished.emplace(path, [=,&session] (Id<Client> orig, bool) {
      expr_ptr->ping(); // TODO how to transmit the max bound information ??
      session.emitMessage(master, session.makeMessage(ctx.mapi.trigger_previous_completed, path));
    });

    comp.OSSIATimeNode()->setExpression(
          std::make_unique<ossia::expression>(
            ossia::expressions::expression_generic{
              std::move(expr)}));
  }
};
*/



/*

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
      session.emitMessage(master, session.makeMessage(ctx.mapi.trigger_entered, path));
    });
    comp.OSSIATimeNode()->leftEvaluation.add_callback([=,&session] {
      session.emitMessage(master, session.makeMessage(ctx.mapi.trigger_left, path));
    });
    comp.OSSIATimeNode()->finishedEvaluation.add_callback([=,&session] (bool b) {
      // b : max bound reached
      session.emitMessage(master, session.makeMessage(ctx.mapi.trigger_finished, path, b));
    });
    comp.OSSIATimeNode()->triggered.add_callback([=,&session] {
      session.emitMessage(master, session.makeMessage(ctx.mapi.trigger_triggered, path));
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
          MixedAsyncUnorderedInGroup{}(ctx, comp, path);
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
          MixedAsyncUnorderedOutOfGroup{}(ctx, comp, path);
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

  //  auto expr = std::make_unique<DateExpression>(
  //        std::chrono::nanoseconds{std::numeric_limits<int64_t>::max()},
  //        comp.makeTrigger());
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

    ctx.doc.noncompensated.network_expressions.emplace(std::move(p), std::move(exp));
  }

}
*/
}
