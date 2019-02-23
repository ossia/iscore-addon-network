#pragma once
#include <score/tools/std/Optional.hpp>

#include <ossia/editor/expression/expression.hpp>

#include <QObject>

#include <chrono>
namespace ossia
{
class time_sync;
}
namespace Network
{
static inline auto get_now()
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::high_resolution_clock::now().time_since_epoch());
}

class DateExpression : public QObject,
                       public ossia::expressions::expression_generic_base
{
public:
  DateExpression();

  void set_min_date(std::chrono::nanoseconds t);

  void update() override;
  bool evaluate() const override;
  void on_first_callback_added(
      ossia::expressions::expression_generic& self) override;
  void on_removing_last_callback(
      ossia::expressions::expression_generic& self) override;

private:
  // The minimal date (in nanosecond epoch) at which this expression shall
  // become true.
  mutable std::chrono::nanoseconds m_minDate{};
  std::chrono::nanoseconds m_curDate{};
  std::function<void()> m_cb;
};

//! this expression is triggered exclusively from an outside source
class AsyncExpression : public ossia::expressions::expression_generic_base
{
public:
  AsyncExpression();

  void ping();

  void update() override;
  bool evaluate() const override;
  void on_first_callback_added(
      ossia::expressions::expression_generic& self) override;
  void on_removing_last_callback(
      ossia::expressions::expression_generic& self) override;

private:
  mutable std::mutex m_mutex;
  mutable bool m_ping{};
  std::function<void()> m_cb;
};

struct expression_with_callback
{
  expression_with_callback(ossia::expression* e) : expr{e} {}
  ossia::expression* expr{};
  optional<ossia::expressions::expression_callback_iterator> it_finished;
};

struct ExprNotInGroup
{
  ExprNotInGroup(ossia::time_sync& n) : node{n} {}
  ossia::time_sync& node;
  optional<ossia::callback_container<std::function<void()>>::iterator>
      it_triggered;

  void cleanTriggerCallback();
};
}
