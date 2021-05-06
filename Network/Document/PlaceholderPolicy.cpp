#include "PlaceholderPolicy.hpp"

#include <score/plugins/documentdelegate/plugin/SerializableDocumentPlugin.hpp>
#include <Network/Session/Session.hpp>
#include <score/tools/Bind.hpp>

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
void JSONReader::read(const Network::EditionPolicy& elt)
{
  obj["SessionId"] = elt.session()->id();
  obj["LocalClient"] = static_cast<Network::Client&>(elt.session()->localClient());

  obj.self.stream.Key("RemoteClients");
  obj.self.stream.StartArray();
  for (auto& clt : elt.session()->remoteClients())
  {
    read(static_cast<Network::Client&>(*clt));
  }
  obj.self.stream.EndArray();
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
void JSONWriter::write(Network::PlaceholderEditionPolicy& elt)
{
  Id<Network::Session> session_id;
  session_id <<= obj["SessionId"];
  JSONObject::Deserializer localClientDeser(obj["LocalClient"]);
  elt.m_session = new Network::Session{
      new Network::LocalClient(localClientDeser, nullptr),
      session_id,
      &elt};

  for (const auto& json_vref : obj["RemoteClients"].toArray())
  {
    JSONObject::Deserializer deser(json_vref);
    elt.m_session->addClient(new Network::RemoteClient(deser, elt.m_session));
  }
}
