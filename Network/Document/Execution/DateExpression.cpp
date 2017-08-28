#include "DateExpression.hpp"
#include <type_traits>
#include <ossia/editor/scenario/time_sync.hpp>
#include <QDebug>
namespace Network
{

DateExpression::DateExpression():
    m_minDate{std::numeric_limits<decltype(m_minDate.count())>::max()}
{

}

void DateExpression::set_min_date(std::chrono::nanoseconds t)
{
  m_minDate = t;
  if(m_curDate < m_minDate && m_cb)
      m_cb();
}

void DateExpression::update()
{
  using namespace std::chrono;
  m_curDate = duration_cast<nanoseconds>(
                high_resolution_clock::now()
                .time_since_epoch());
}

bool DateExpression::evaluate() const
{
  bool val = m_curDate < m_minDate;
  if(val)
  {
    m_minDate = std::chrono::nanoseconds{std::numeric_limits<decltype(m_minDate.count())>::max()};
  }
  return val;
}

void DateExpression::on_first_callback_added(
    ossia::expressions::expression_generic& self)
{
    m_cb = [&] { self.send(m_curDate < m_minDate); };
}

void DateExpression::on_removing_last_callback(
    ossia::expressions::expression_generic& self)
{
    m_cb = {};
}




AsyncExpression::AsyncExpression()
{

}

void AsyncExpression::ping()
{
  qDebug("ping");
  m_mutex.lock();
  m_ping = true;
  m_mutex.unlock();
  if(m_cb)
    m_cb();
}

void AsyncExpression::update()
{
}

bool AsyncExpression::evaluate() const
{
  m_mutex.lock();
  bool val = m_ping;

  qDebug() << "evaluate" << val;
  if(val)
  {
      // Reset the status for loops.
      m_ping = false;
  }
  m_mutex.unlock();
  return val;
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

void ExprNotInGroup::cleanTriggerCallback()
{
    if(it_triggered)
    {
        node.triggered.remove_callback(*it_triggered);
        it_triggered = ossia::none;
    }
}

}
