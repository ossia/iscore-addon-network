#pragma once
#include <Scenario/Document/Interval/IntervalExecution.hpp>
#include <Scenario/Document/TimeSync/TimeSyncExecution.hpp>
#include <Scenario/Process/Algorithms/Accessors.hpp>
#include <Scenario/Process/ScenarioModel.hpp>

#include <score/model/path/PathSerialization.hpp>

#include <ossia/editor/scenario/time_sync.hpp>

#include <Network/Document/Execution/DateExpression.hpp>
#include <Network/Document/Execution/FreeScenarioPolicy.hpp>
#include <Network/Document/Execution/MixedScenarioPolicy.hpp>
#include <Network/Session/Session.hpp>

namespace Network
{

struct NonCompensatedExpressionInGroup
{
  struct ExprData
  {
    ExprData(ossia::time_sync& n) : node{n} {}
    ossia::time_sync& node;
    std::shared_ptr<expression_with_callback> shared_expr;
    AsyncExpression* async_expr{};

    std::optional<ossia::callback_container<std::function<void()>>::iterator>
        it_triggered;
  };

  std::shared_ptr<ExprData> setupExpr(Execution::TimeSyncComponent& comp)
  {
    // Wrap the expresion
    auto e = std::make_shared<ExprData>(*comp.OSSIATimeSync());
    e->shared_expr = std::make_shared<expression_with_callback>(
        comp.makeTrigger().release());
    e->async_expr = new AsyncExpression;

    // ossia::expressions::expression_generic{
    //    std::unique_ptr<ossia::expressions::expression_generic_base>(e->async_expr)};
    comp.OSSIATimeSync()->set_expression(std::make_unique<ossia::expression>(
        ossia::in_place_type<ossia::expressions::expression_generic>,
        std::unique_ptr<ossia::expressions::expression_generic_base>(
            e->async_expr)));

    return e;
  }
};

struct SharedNonCompensatedAsyncInGroup
    : public NonCompensatedExpressionInGroup
{
  void operator()(
      NetworkPrunerContext& ctx,
      Execution::TimeSyncComponent& comp,
      const Path<Scenario::TimeSyncModel>& path)
  {
    qDebug() << "SharedNonCompensatedAsyncInGroup";
    auto e = setupExpr(comp);

    // Then set specific callbacks for outside events
    auto& session = ctx.session;
    auto master = ctx.master;
    auto& mapi = ctx.mapi;

    // When the trigger enters evaluation
    ctx.doc.noncompensated.trigger_evaluation_entered.emplace(
        path, [=, &session, &mapi](const Id<Client>& orig) {
          qDebug() << "Trigger entered evaluation";
          if (!e->it_triggered.has_value())
          {
            qDebug() << "Adding a 'triggered' callback";
            e->it_triggered
                = e->node.triggered.add_callback([=, &mapi, &session] {
                    qDebug("SharedScenarioPolicy: trigger triggered");
                    session.emitMessage(
                        master,
                        session.makeMessage(mapi.trigger_triggered, path));
                  });
          }

          if (!e->shared_expr->it_finished.has_value())
          {
            qDebug() << "Registering callback";
            // TODO what if this trigger's condition already had become true ?
            e->shared_expr->it_finished = ossia::expressions::add_callback(
                *e->shared_expr->expr, [=, &session, &mapi](bool b) {
                  qDebug() << "Evaluation true" << b;
                  if (b)
                  {
                    session.emitMessage(
                        master,
                        session.makeMessage(
                            mapi.trigger_expression_true, path));
                  }
                });
          }
        });

    // When the trigger finishes evaluation
    ctx.doc.noncompensated.trigger_evaluation_finished.emplace(
        path, [=](const Id<Client>& orig, bool b) {
          qDebug() << "Evaluation finished" << b;
          if (e->it_triggered)
          {
            e->node.triggered.remove_callback(*e->it_triggered);
            e->it_triggered = std::nullopt;
          }

          if (e->shared_expr->it_finished)
          {
            ossia::expressions::remove_callback(
                *e->shared_expr->expr, *e->shared_expr->it_finished);

            e->shared_expr->it_finished = std::nullopt;
          }

          e->async_expr
              ->ping(); // TODO how to transmit the max bound information ??
        });

    // When the trigger can be triggered
    ctx.doc.noncompensated.trigger_triggered.emplace(
        path, [=](const Id<Client>& orig) {
          qDebug() << "Triggered";
          if (e->it_triggered)
          {
            e->node.triggered.remove_callback(*e->it_triggered);
            e->it_triggered = std::nullopt;
          }

          if (e->shared_expr->it_finished)
          {
            ossia::expressions::remove_callback(
                *e->shared_expr->expr, *e->shared_expr->it_finished);

            e->shared_expr->it_finished = std::nullopt;
          }

          e->async_expr->ping();
        });
  }
};

struct SharedNonCompensatedSyncInGroup : public NonCompensatedExpressionInGroup
{
  void operator()(
      NetworkPrunerContext& ctx,
      Execution::TimeSyncComponent& comp,
      const Path<Scenario::TimeSyncModel>& path)
  {
    qDebug() << "SharedNonCompensatedSyncInGroup";
    auto e = setupExpr(comp);

    // Then set specific callbacks for outside events
    auto& session = ctx.session;
    auto& mapi = ctx.mapi;
    auto master = ctx.master;

    // When the trigger enters evaluation
    ctx.doc.noncompensated.trigger_evaluation_entered.emplace(
        path, [=, &session, &mapi](const Id<Client>& orig) {
          if (!e->it_triggered)
          {
            e->it_triggered
                = e->node.triggered.add_callback([=, &mapi, &session] {
                    qDebug("SharedScenarioPolicy: trigger triggered");
                    session.emitMessage(
                        master,
                        session.makeMessage(mapi.trigger_triggered, path));
                  });
          }

          if (!e->shared_expr->it_finished)
          {
            e->shared_expr->it_finished = ossia::expressions::add_callback(
                *e->shared_expr->expr, [=, &session](bool b) {
                  if (b)
                  {
                    session.emitMessage(
                        master,
                        session.makeMessage(
                            mapi.trigger_expression_true, path));
                  }
                });
          }
        });

    // When the trigger finishes evaluation
    ctx.doc.noncompensated.trigger_evaluation_finished.emplace(
        path, [=, &session](const Id<Client>& orig, bool b) {
          if (e->it_triggered)
          {
            e->node.triggered.remove_callback(*e->it_triggered);
            e->it_triggered = std::nullopt;
          }

          if (e->shared_expr->it_finished)
          {
            ossia::expressions::remove_callback(
                *e->shared_expr->expr, *e->shared_expr->it_finished);

            e->shared_expr->it_finished = std::nullopt;
          }
          e->async_expr
              ->ping(); // TODO how to transmit the max bound information ??

          // Since we're ordered, we inform the master when we're ready to
          // trigger the followers
          session.emitMessage(
              master,
              session.makeMessage(mapi.trigger_previous_completed, path));
        });

    // When the trigger can be triggered
    ctx.doc.noncompensated.trigger_triggered.emplace(
        path, [=, &session](const Id<Client>& orig) {
          if (e->it_triggered)
          {
            e->node.triggered.remove_callback(*e->it_triggered);
            e->it_triggered = std::nullopt;
          }

          if (e->shared_expr->it_finished)
          {
            ossia::expressions::remove_callback(
                *e->shared_expr->expr, *e->shared_expr->it_finished);

            e->shared_expr->it_finished = std::nullopt;
          }
          e->async_expr->ping();

          // Since we're ordered, we inform the master when we're ready to
          // trigger the followers
          session.emitMessage(
              master,
              session.makeMessage(mapi.trigger_previous_completed, path));
        });
  }
};

struct SharedNonCompensatedAsyncOutOfGroup
{
  void operator()(
      NetworkPrunerContext& ctx,
      Execution::TimeSyncComponent& comp,
      const Path<Scenario::TimeSyncModel>& path)
  {
    qDebug() << "SharedNonCompensatedAsyncOutOfGroup";
    auto& session = ctx.session;
    auto& mapi = ctx.mapi;
    auto master = ctx.master;

    auto e = std::make_shared<ExprNotInGroup>(*comp.OSSIATimeSync());
    auto expr = std::make_unique<AsyncExpression>();
    auto expr_ptr = expr.get();

    ctx.doc.noncompensated.trigger_evaluation_entered.emplace(
        path, [=, &session, &mapi](const Id<Client>& orig) {
          if (!e->it_triggered)
          {
            e->it_triggered
                = e->node.triggered.add_callback([=, &mapi, &session] {
                    qDebug("SharedScenarioPolicy: trigger triggered");
                    session.emitMessage(
                        master,
                        session.makeMessage(mapi.trigger_triggered, path));
                  });
          }
        });

    ctx.doc.noncompensated.trigger_triggered.emplace(
        path, [=](const Id<Client>& orig) {
          e->cleanTriggerCallback();
          expr_ptr->ping();
        });
    ctx.doc.noncompensated.trigger_evaluation_finished.emplace(
        path, [=](const Id<Client>& orig, bool) {
          e->cleanTriggerCallback();
          expr_ptr
              ->ping(); // TODO how to transmit the max bound information ??
        });

    comp.OSSIATimeSync()->set_expression(std::make_unique<ossia::expression>(
        ossia::in_place_type<ossia::expressions::expression_generic>,
        std::move(expr)));
  }
};

struct SharedNonCompensatedSyncOutOfGroup
{
  void operator()(
      NetworkPrunerContext& ctx,
      Execution::TimeSyncComponent& comp,
      const Path<Scenario::TimeSyncModel>& path)
  {
    qDebug() << "SharedNonCompensatedSyncOutOfGroup";
    auto expr = std::make_unique<AsyncExpression>();
    auto expr_ptr = expr.get();
    auto& session = ctx.session;
    auto& mapi = ctx.mapi;
    auto master = ctx.master;

    auto e = std::make_shared<ExprNotInGroup>(*comp.OSSIATimeSync());
    ctx.doc.noncompensated.trigger_evaluation_entered.emplace(
        path, [=, &session, &mapi](const Id<Client>& orig) {
          if (!e->it_triggered)
          {
            e->it_triggered
                = e->node.triggered.add_callback([=, &mapi, &session] {
                    qDebug("SharedScenarioPolicy: trigger triggered");
                    session.emitMessage(
                        master,
                        session.makeMessage(mapi.trigger_triggered, path));
                  });
          }
        });

    ctx.doc.noncompensated.trigger_triggered.emplace(
        path, [=, &session, &mapi](const Id<Client>& orig) {
          e->cleanTriggerCallback();

          expr_ptr->ping();
          session.emitMessage(
              master,
              session.makeMessage(mapi.trigger_previous_completed, path));
        });
    ctx.doc.noncompensated.trigger_evaluation_finished.emplace(
        path, [=, &session, &mapi](const Id<Client>& orig, bool) {
          e->cleanTriggerCallback();

          expr_ptr
              ->ping(); // TODO how to transmit the max bound information ??
          session.emitMessage(
              master,
              session.makeMessage(mapi.trigger_previous_completed, path));
        });

    comp.OSSIATimeSync()->set_expression(std::make_unique<ossia::expression>(
        ossia::in_place_type<ossia::expressions::expression_generic>,
        std::move(expr)));
  }
};
}
