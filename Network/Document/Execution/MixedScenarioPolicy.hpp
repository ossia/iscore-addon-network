#pragma once
#include <Network/Document/Execution/Context.hpp>

namespace Network
{

struct MixedScenarioPolicy
{
  NetworkPrunerContext& ctx;

  void operator()(
      Engine::Execution::ProcessComponent& comp,
      Scenario::ScenarioInterface& ip,
      const Group& cur);

  void operator()(Engine::Execution::TimeNodeComponent& comp, const Group& parent_group)
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
            doc.trigger_evaluation_entered[path] = [=] {
              base_expr->it = ossia::expressions::add_callback(
                    *base_expr->expr,
                    [] (bool b) {
                if(b)
                {
                  // Send message to master

                }
              });
            };

            doc.trigger_evaluation_finished[path] = [=] (bool b) {
              if(base_expr->it)
                ossia::expressions::remove_callback(
                      *base_expr->expr, *base_expr->it);

              expr_ptr->ping(); // TODO how to transmit the max bound information ??
            };

            doc.trigger_triggered[path] = [=] {
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

            doc.trigger_triggered[path] = [=] {
              expr_ptr->ping();
            };
            doc.trigger_evaluation_finished[path] = [=] (bool) {
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
};


}
