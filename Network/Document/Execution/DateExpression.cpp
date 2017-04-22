#include "DateExpression.hpp"

namespace Network
{

DateExpression::DateExpression(
    std::chrono::nanoseconds t,
    ossia::expression_ptr expr):
  m_minDate{t}
, m_expression{std::move(expr)}
{

}

void DateExpression::update()
{
  using namespace std::chrono;
  m_curDate = duration_cast<nanoseconds>(
                high_resolution_clock::now()
                .time_since_epoch());

  if(m_expression)
    ossia::expressions::update(*m_expression);
}

bool DateExpression::evaluate() const
{
  if(m_curDate < m_minDate)
    return false;
  return m_expression
      ? ossia::expressions::evaluate(*m_expression)
      : true;
}

void DateExpression::on_first_callback_added(
    ossia::expressions::expression_generic& self)
{
  // start first expression observation
  if(m_expression)
  {
    m_callback =
        ossia::expressions::add_callback(
          *m_expression,
          [&] (bool result) {
      self.send(evaluate_callback(result));
    });
  }

}

void DateExpression::on_removing_last_callback(
    ossia::expressions::expression_generic& self)
{
  if(m_expression)
  {
    ossia::expressions::remove_callback(
          *m_expression,
          m_callback);
  }
}

bool DateExpression::evaluate_callback(
    bool res)
{
  if(m_curDate < m_minDate)
    return false;
  return res;
}






AsyncExpression::AsyncExpression()
{

}

void AsyncExpression::ping()
{
  m_ping = true;
  if(m_cb)
    m_cb();
}

void AsyncExpression::update()
{
}

bool AsyncExpression::evaluate() const
{
  return m_ping;
}

void AsyncExpression::on_first_callback_added(
    ossia::expressions::expression_generic& self)
{
  m_cb = [&] { self.send(m_ping); };
}

void AsyncExpression::on_removing_last_callback(
    ossia::expressions::expression_generic& self)
{
  m_cb = {};
}

}
