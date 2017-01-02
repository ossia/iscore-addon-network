#include <iscore/tools/std/Optional.hpp>
#include <iscore/serialization/DataStreamVisitor.hpp>
#include <iscore/serialization/JSONVisitor.hpp>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <algorithm>

#include <Network/Document/DocumentPlugin.hpp>
#include "PlaceholderPolicy.hpp"
#include <iscore/serialization/JSONValueVisitor.hpp>
#include <iscore/model/Identifier.hpp>
#include <Network/Client/LocalClient.hpp>
#include <Network/Client/RemoteClient.hpp>
#include <Network/Session/Session.hpp>


template <>
void DataStreamReader::read(
        const Network::NetworkPolicy& elt)
{
    m_stream << elt.session()->id();
    readFrom(static_cast<Network::Client&>(elt.session()->localClient()));

    m_stream << elt.session()->remoteClients().count();
    for(auto& clt : elt.session()->remoteClients())
    {
        readFrom(static_cast<Network::Client&>(*clt));
    }

    insertDelimiter();
}

template <>
void JSONObjectReader::read(
        const Network::NetworkPolicy& elt)
{
    obj["SessionId"] = toJsonValue(elt.session()->id());
    obj["LocalClient"] = toJsonObject(static_cast<Network::Client&>(elt.session()->localClient()));

    QJsonArray arr;
    for(auto& clt : elt.session()->remoteClients())
    {
        arr.push_back(toJsonObject(static_cast<Network::Client&>(*clt)));
    }
    obj["RemoteClients"] = arr;
}


template <>
void DataStreamWriter::write(
        Network::PlaceholderNetworkPolicy& elt)
{
    Id<Network::Session> sessId;
    m_stream >> sessId;

    elt.m_session = new Network::Session{new Network::LocalClient(*this, nullptr), sessId, &elt};

    int n;
    m_stream >> n;
    for(; n --> 0;)
    {
        elt.m_session->addClient(new Network::RemoteClient(*this, elt.m_session));
    }

    checkDelimiter();
}

template <>
void JSONObjectWriter::write(
        Network::PlaceholderNetworkPolicy& elt)
{
    JSONObject::Deserializer localClientDeser(obj["LocalClient"].toObject());
    elt.m_session = new Network::Session{
            new Network::LocalClient(localClientDeser, nullptr),
            fromJsonValue<Id<Network::Session>>(obj["SessionId"]),
            &elt};

    for(const auto& json_vref : obj["RemoteClients"].toArray())
    {
        JSONObject::Deserializer deser(json_vref.toObject());
        elt.m_session->addClient(new Network::RemoteClient(deser, elt.m_session));
    }

}
