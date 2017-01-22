#pragma once
#include <QObject>
#include <ossia/editor/expression/expression.hpp>
#include <iscore/tools/std/Optional.hpp>
#include <chrono>
namespace Network
{
class DateExpression :
    public QObject,
    public ossia::expressions::expression_generic_base
{
  Q_OBJECT
  public:
    DateExpression(std::chrono::nanoseconds t,
                   ossia::expression_ptr expr);

    void update() override;
    bool evaluate() const override;
    void onFirstCallbackAdded(ossia::expressions::expression_generic& self) override;
    void onRemovingLastCallback(ossia::expressions::expression_generic& self) override;

signals:
    void subexpressionReady();

private:
    bool evaluate_callback(bool res);

    // The minimal date (in nanosecond epoch) at which this expression shall become true.
    std::chrono::nanoseconds m_minDate{};
    std::chrono::nanoseconds m_curDate{};
    ossia::expression_ptr m_expression{};
    ossia::expressions::expression_callback_iterator m_callback;
};

//! this expression is triggered exclusively from an outside source
class AsyncExpression :
    public ossia::expressions::expression_generic_base
{
  public:
    AsyncExpression();

    void ping();

    void update() override;
    bool evaluate() const override;
    void onFirstCallbackAdded(ossia::expressions::expression_generic& self) override;
    void onRemovingLastCallback(ossia::expressions::expression_generic& self) override;

private:
    std::atomic_bool m_ping{};
    std::function<void()> m_cb;
};

struct expression_with_callback
{
  expression_with_callback(ossia::expression* e): expr{e} { }
  ossia::expression* expr{};
  optional<ossia::expressions::expression_callback_iterator> it;
};

}
