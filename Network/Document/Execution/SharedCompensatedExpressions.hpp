#pragma once
#include <Network/Session/Session.hpp>
#include <Network/Document/Execution/DateExpression.hpp>
#include <Network/Document/Execution/MixedScenarioPolicy.hpp>
#include <Network/Document/Execution/FreeScenarioPolicy.hpp>
#include <Scenario/Process/ScenarioModel.hpp>
#include <Scenario/Process/Algorithms/Accessors.hpp>
#include <Scenario/Document/TimeSync/TimeSyncExecution.hpp>
#include <Scenario/Document/Interval/IntervalExecution.hpp>
#include <score/model/path/PathSerialization.hpp>
#include <ossia/editor/scenario/time_sync.hpp>

namespace Network
{
struct CompensatedExpressionInGroup
{
    struct ExprData {
        ExprData(ossia::time_sync& n): node{n} { }
        ossia::time_sync& node;
        std::shared_ptr<expression_with_callback> shared_expr;
        DateExpression* date_expr{};

        void cleanTriggerCallback()
        {
            if(it_triggered)
            {
                node.triggered.remove_callback(*it_triggered);
                it_triggered = ossia::none;
            }
        }

        optional<ossia::callback_container<std::function<void()>>::iterator> it_triggered;
    };

    auto setupExpr(
            Execution::TimeSyncComponent& comp)
    {
        // Wrap the expresion
        auto e = std::make_shared<ExprData>(*comp.OSSIATimeSync());
        e->shared_expr = std::make_shared<expression_with_callback>(comp.makeTrigger().release());
        e->date_expr = new DateExpression;

        comp.OSSIATimeSync()->set_expression(
                    std::make_unique<ossia::expression>(
                           eggs::variants::in_place<ossia::expressions::expression_generic>,
                           std::unique_ptr<ossia::expressions::expression_generic_base>(e->date_expr))
                    );

        return e;
    }
};

struct SharedCompensatedAsyncInGroup : public CompensatedExpressionInGroup
{
    void operator()(
            NetworkPrunerContext& ctx,
            Execution::TimeSyncComponent& comp,
            const Path<Scenario::TimeSyncModel>& path)
    {
        qDebug() << "SharedCompensatedAsyncInGroup";
        auto e = setupExpr(comp);

        // Then set specific callbacks for outside events
        auto& session = ctx.session;
        auto master = ctx.master;
        auto& mapi = ctx.mapi;

        // When the trigger enters evaluation
        ctx.doc.noncompensated.trigger_evaluation_entered.emplace(
                    path,
                    [=,&session,&mapi] (const Id<Client>& orig) {
            if(!e->it_triggered)
            {
                e->it_triggered = e->node.triggered.add_callback([=,&mapi,&session] {
                    qDebug("SharedScenarioPolicy: trigger triggered");
                    session.emitMessage(master, session.makeMessage(mapi.trigger_triggered, path));
                });
            }

            if(!e->shared_expr->it_finished)
            {
                e->shared_expr->it_finished = ossia::expressions::add_callback(
                            *e->shared_expr->expr,
                            [=,&session,&mapi] (bool b) {
                    qDebug() << "Evaluation entered" << b;
                    if(b)
                    {
                        session.emitMessage(
                                    master,
                                    session.makeMessage(mapi.trigger_expression_true, path));
                    }
                });
            }
        });

        // When the trigger finishes evaluation
        ctx.doc.noncompensated.trigger_evaluation_finished.emplace(path, [=] (const Id<Client>& orig, bool b) {
            qDebug() << "Evaluation finished" << b;
            e->cleanTriggerCallback();

            if(e->shared_expr->it_finished)
            {
                ossia::expressions::remove_callback(*e->shared_expr->expr, *e->shared_expr->it_finished);

                e->shared_expr->it_finished = ossia::none;
            }

            e->date_expr->set_min_date(get_now()); // TODO how to transmit the max bound information ??
        });

        // When the trigger can be triggered
        ctx.doc.compensated.trigger_triggered.emplace(path, [=] (const Id<Client>& orig, qint64 ns) {
            qDebug() << "Triggered";
            e->cleanTriggerCallback();

            if(e->shared_expr->it_finished)
            {
                ossia::expressions::remove_callback(
                            *e->shared_expr->expr, *e->shared_expr->it_finished);

                e->shared_expr->it_finished = ossia::none;
            }

            e->date_expr->set_min_date(std::chrono::nanoseconds(ns));
        });
    }
};


struct SharedCompensatedSyncInGroup : public CompensatedExpressionInGroup
{
    void operator()(
            NetworkPrunerContext& ctx,
            Execution::TimeSyncComponent& comp,
            const Path<Scenario::TimeSyncModel>& path)
    {
        qDebug() << "SharedCompensatedSyncInGroup";
        auto e = setupExpr(comp);

        // Then set specific callbacks for outside events
        auto& session = ctx.session;
        auto& mapi = ctx.mapi;
        auto master = ctx.master;

        // When the trigger enters evaluation
        ctx.doc.noncompensated.trigger_evaluation_entered.emplace(
                    path,
                    [=,&session,&mapi] (const Id<Client>& orig) {

            if(!e->it_triggered)
            {
                e->it_triggered = e->node.triggered.add_callback([=,&mapi,&session] {
                    qDebug("SharedScenarioPolicy: trigger triggered");
                    session.emitMessage(master, session.makeMessage(mapi.trigger_triggered, path));
                });
            }

            if(!e->shared_expr->it_finished)
            {
                e->shared_expr->it_finished = ossia::expressions::add_callback(
                            *e->shared_expr->expr,
                            [=,&session] (bool b) {
                    if(b)
                    {
                        session.emitMessage(
                                    master,
                                    session.makeMessage(mapi.trigger_expression_true, path));
                    }
                });
            }
        });

        // When the trigger finishes evaluation
        ctx.doc.noncompensated.trigger_evaluation_finished.emplace(
                    path,
                    [=,&session] (const Id<Client>& orig, bool b) {
            e->cleanTriggerCallback();
            if(e->shared_expr->it_finished)
            {
                ossia::expressions::remove_callback(*e->shared_expr->expr, *e->shared_expr->it_finished);

                e->shared_expr->it_finished = ossia::none;
            }
            e->date_expr->set_min_date(get_now());  // TODO how to transmit the max bound information ??

            // Since we're ordered, we inform the master when we're ready to trigger the followers
            session.emitMessage(master, session.makeMessage(mapi.trigger_previous_completed, path));
        });

        // When the trigger can be triggered
        ctx.doc.compensated.trigger_triggered.emplace(
                    path,
                    [=,&session] (const Id<Client>& orig, qint64 ns) {
            e->cleanTriggerCallback();

            if(e->shared_expr->it_finished)
            {
                ossia::expressions::remove_callback(
                            *e->shared_expr->expr, *e->shared_expr->it_finished);

                e->shared_expr->it_finished = ossia::none;
            }
            e->date_expr->set_min_date(std::chrono::nanoseconds(ns));

            // Since we're ordered, we inform the master when we're ready to trigger the followers
            session.emitMessage(master, session.makeMessage(mapi.trigger_previous_completed, path));
        });

    }
};


struct SharedCompensatedAsyncOutOfGroup
{
    void operator()(
            NetworkPrunerContext& ctx,
            Execution::TimeSyncComponent& comp,
            const Path<Scenario::TimeSyncModel>& path)
    {
        qDebug() << "SharedCompensatedAsyncOutOfGroup";
        auto expr = std::make_unique<DateExpression>();
        DateExpression* expr_ptr = expr.get();

        auto& session = ctx.session;
        auto& mapi = ctx.mapi;
        auto master = ctx.master;

        auto e = std::make_shared<ExprNotInGroup>(*comp.OSSIATimeSync());
        ctx.doc.noncompensated.trigger_evaluation_entered.emplace(
                    path,
                    [=,&session,&mapi] (const Id<Client>& orig) {
            if(!e->it_triggered)
            {
                e->it_triggered = e->node.triggered.add_callback([=,&mapi,&session] {
                    qDebug("SharedScenarioPolicy: trigger triggered");
                    session.emitMessage(master, session.makeMessage(mapi.trigger_triggered, path));
                });
            }
        });

        ctx.doc.compensated.trigger_triggered.emplace(path, [=] (const Id<Client>& orig, qint64 ns) {
            e->cleanTriggerCallback();
            expr_ptr->set_min_date(std::chrono::nanoseconds(ns));
        });
        ctx.doc.noncompensated.trigger_evaluation_finished.emplace(path, [=] (const Id<Client>& orig, bool) {

            e->cleanTriggerCallback();
            expr_ptr->set_min_date(get_now()); // TODO how to transmit the max bound information ??
        });

        comp.OSSIATimeSync()->set_expression(
                    std::make_unique<ossia::expression>(
                        eggs::variants::in_place<ossia::expressions::expression_generic>,
                        std::move(expr)));
    }
};


struct SharedCompensatedSyncOutOfGroup
{
    void operator()(
            NetworkPrunerContext& ctx,
            Execution::TimeSyncComponent& comp,
            const Path<Scenario::TimeSyncModel>& path)
    {
        qDebug() << "SharedCompensatedSyncOutOfGroup";
        auto expr = std::make_unique<DateExpression>();
        DateExpression* expr_ptr = expr.get();
        auto& session = ctx.session;
        auto& mapi = ctx.mapi;
        auto master = ctx.master;

        auto e = std::make_shared<ExprNotInGroup>(*comp.OSSIATimeSync());
        ctx.doc.noncompensated.trigger_evaluation_entered.emplace(
                    path,
                    [=,&session,&mapi] (const Id<Client>& orig) {
            if(!e->it_triggered)
            {
                e->it_triggered = e->node.triggered.add_callback([=,&mapi,&session] {
                    qDebug("SharedScenarioPolicy: trigger triggered");
                    session.emitMessage(master, session.makeMessage(mapi.trigger_triggered, path));
                });
            }
        });

        ctx.doc.compensated.trigger_triggered.emplace(path, [=,&session,&mapi] (const Id<Client>& orig, qint64 ns) {
            e->cleanTriggerCallback();

            expr_ptr->set_min_date(std::chrono::nanoseconds(ns));
            session.emitMessage(master, session.makeMessage(mapi.trigger_previous_completed, path));
        });
        ctx.doc.noncompensated.trigger_evaluation_finished.emplace(path, [=,&session,&mapi] (const Id<Client>& orig, bool) {
            e->cleanTriggerCallback();

            expr_ptr->set_min_date(get_now()); // TODO how to transmit the max bound information ??
            session.emitMessage(master, session.makeMessage(mapi.trigger_previous_completed, path));
        });

        comp.OSSIATimeSync()->set_expression(
                    std::make_unique<ossia::expression>(
                            eggs::variants::in_place<ossia::expressions::expression_generic>,
                            std::move(expr)));
    }
};

}
