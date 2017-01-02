#include <Network/Communication/MessageMapper.hpp>
#include <Network/Communication/MessageValidator.hpp>
#include <iscore/tools/std/Optional.hpp>

#include <Network/Communication/NetworkMessage.hpp>
#include "Session.hpp"
#include <Network/Client/LocalClient.hpp>

class QObject;

namespace Network
{
Session::Session(LocalClient* client, Id<Session> id, QObject* parent):
    IdentifiedObject<Session>{id, "Session", parent},
    m_client{client},
    m_mapper{new MessageMapper},
    m_validator{new MessageValidator(*this, mapper())}
{
    m_client->setParent(this);
}

Session::~Session()
{
    delete m_mapper;
    delete m_validator;
}

NetworkMessage Session::makeMessage(const QByteArray& address)
{
    NetworkMessage m;
    m.address = address;
    m.clientId = localClient().id().val();
    m.sessionId = id().val();

    return m;
}


void Session::validateMessage(NetworkMessage m)
{
    if(validator().validate(m))
        mapper().map(m);
}
}
