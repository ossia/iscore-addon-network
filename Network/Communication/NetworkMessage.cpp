#include "NetworkMessage.hpp"

#include <score/model/Identifier.hpp>
#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>

#include <core/document/DocumentModel.hpp>

#include <QDataStream>

#include <Network/Client/LocalClient.hpp>
#include <Network/Session/Session.hpp>
SCORE_SERALIZE_DATASTREAM_DEFINE(Id<Network::Session>)
SCORE_SERALIZE_DATASTREAM_DEFINE(Id<Network::Client>)

namespace Network
{
QDataStream& operator<<(QDataStream& s, const Network::NetworkMessage& m)
{
  s << m.address << m.sessionId << m.clientId << m.data;
  return s;
}

QDataStream& operator>>(QDataStream& s, Network::NetworkMessage& m)
{
  s >> m.address >> m.sessionId >> m.clientId >> m.data;
  return s;
}
NetworkMessage::NetworkMessage(Network::Session& s, QByteArray&& addr, QByteArray&& data)
    : address{addr}
    , sessionId{s.id()}
    , clientId{s.localClient().id()}
    , data{data}
{
}
}
