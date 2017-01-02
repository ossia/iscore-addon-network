#include <QList>

#include <Network/Session/Session.hpp>
#include "MessageValidator.hpp"
#include <Network/Communication/MessageMapper.hpp>
#include <Network/Communication/NetworkMessage.hpp>
#include <iscore/model/Identifier.hpp>

namespace Network
{
MessageValidator::MessageValidator(Session& s, MessageMapper& map):
    m_session{s},
    m_mapper{map}
{

}

bool MessageValidator::validate(NetworkMessage m)
{
    return  m_mapper.addresses().contains(m.address)
            && m_session.id().val() == m.sessionId;
}
}
