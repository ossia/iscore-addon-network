#include "MessageMapper.hpp"

#include <score/tools/Debug.hpp>

#include <QDataStream>
#include <QDebug>

namespace Network
{
void MessageMapper::addHandler(
    const QObject* owner, QByteArray addr,
    std::function<void(const NetworkMessage&)> fun)
{
  SCORE_ASSERT(!contains(addr));
  m_handlers[std::move(addr)] = Handler{owner, std::move(fun)};
}

void MessageMapper::map(const NetworkMessage& m)
{
  auto it = m_handlers.find(m.address);
  if(it == m_handlers.end())
  {
    qDebug() << "Address" << m.address << "not handled.";
    return;
  }

  if(!it->second.owner)
  {
    // Whoever installed this is gone; so is anything it captured.
    m_handlers.erase(it);
    return;
  }

  (it->second.fun)(m);
}

bool MessageMapper::contains(const QByteArray& b) const
{
  return m_handlers.find(b) != m_handlers.end();
}
}
