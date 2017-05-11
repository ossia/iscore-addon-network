#pragma once
#include <iscore/model/Identifier.hpp>

namespace Network
{
class MessageMapper;
class Session;
struct NetworkMessage;

class MessageValidator
{
    public:
        MessageValidator(Id<Session> s, MessageMapper& map);

        bool validate(const NetworkMessage& m);

    private:
        Id<Session> m_session;
        MessageMapper& m_mapper;
};
}
