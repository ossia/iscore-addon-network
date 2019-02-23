#include "MessageMapper.hpp"

#include <score/tools/Todo.hpp>

#include <QDataStream>
#include <QDebug>

namespace Network
{
void MessageMapper::addHandler(
    QByteArray addr,
    std::function<void(const NetworkMessage&)> fun)
{
  SCORE_ASSERT(!contains(addr));
  m_handlers[std::move(addr)] = std::move(fun);
}

void MessageMapper::map(const NetworkMessage& m)
{
  auto it = m_handlers.find(m.address);
  if (it != m_handlers.end())
    (it.value())(m);
  else
    qDebug() << "Address" << m.address << "not handled.";
}

bool MessageMapper::contains(const QByteArray& b) const
{
  return m_handlers.find(b) != m_handlers.end();
}
}
