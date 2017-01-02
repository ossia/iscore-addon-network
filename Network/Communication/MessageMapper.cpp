#include <QDebug>
#include <iscore/tools/Todo.hpp>
#include "MessageMapper.hpp"
#include <Network/Communication/NetworkMessage.hpp>

namespace Network
{

void MessageMapper::addHandler(QByteArray addr, std::function<void(NetworkMessage)> fun)
{
    ISCORE_ASSERT(!contains(addr));
    m_handlers[addr] = fun;
}

void MessageMapper::map(NetworkMessage m)
{
    auto it = m_handlers.find(m.address);
    if(it != m_handlers.end())
        (it.value())(m);
    else
      qDebug() << "Address" << m.address << "not handled.";
}

bool MessageMapper::contains(const QByteArray& b) const
{
  return m_handlers.find(b) != m_handlers.end();
}


}
