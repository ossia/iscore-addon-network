#include "PlaceholderPolicy.hpp"

#include <score/plugins/documentdelegate/plugin/SerializableDocumentPlugin.hpp>
#include <Network/Session/Session.hpp>

template <>
void DataStreamReader::read(const Network::EditionPolicy& elt)
{
  m_stream << elt.session()->id();
  readFrom(static_cast<Network::Client&>(elt.session()->localClient()));

  m_stream << elt.session()->remoteClients().count();
  for (auto& clt : elt.session()->remoteClients())
  {
    readFrom(static_cast<Network::Client&>(*clt));
  }

  insertDelimiter();
}

template <>
void JSONObjectReader::read(const Network::EditionPolicy& elt)
{
  obj["SessionId"] = toJsonValue(elt.session()->id());
  obj["LocalClient"] = toJsonObject(
      static_cast<Network::Client&>(elt.session()->localClient()));

  QJsonArray arr;
  for (auto& clt : elt.session()->remoteClients())
  {
    arr.push_back(toJsonObject(static_cast<Network::Client&>(*clt)));
  }
  obj["RemoteClients"] = arr;
}

template <>
void DataStreamWriter::write(Network::PlaceholderEditionPolicy& elt)
{
  Id<Network::Session> sessId;
  m_stream >> sessId;

  elt.m_session = new Network::Session{
      new Network::LocalClient(*this, nullptr), sessId, &elt};

  int n;
  m_stream >> n;
  for (; n-- > 0;)
  {
    elt.m_session->addClient(new Network::RemoteClient(*this, elt.m_session));
  }

  checkDelimiter();
}

template <>
void JSONObjectWriter::write(Network::PlaceholderEditionPolicy& elt)
{
  JSONObject::Deserializer localClientDeser(obj["LocalClient"].toObject());
  elt.m_session = new Network::Session{
      new Network::LocalClient(localClientDeser, nullptr),
      fromJsonValue<Id<Network::Session>>(obj["SessionId"]),
      &elt};

  for (const auto& json_vref : obj["RemoteClients"].toArray())
  {
    JSONObject::Deserializer deser(json_vref.toObject());
    elt.m_session->addClient(new Network::RemoteClient(deser, elt.m_session));
  }
}
