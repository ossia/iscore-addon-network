#include "MessageValidator.hpp"

#include <Network/Communication/MessageMapper.hpp>
#include <Network/Communication/NetworkMessage.hpp>

namespace Network
{
MessageValidator::MessageValidator(Id<Session> s, MessageMapper& map)
    : m_session{s}
    , m_mapper{map}
{
}

bool MessageValidator::validate(const NetworkMessage& m)
{
  return m_mapper.contains(m.address) && m_session == m.sessionId;
}
}
