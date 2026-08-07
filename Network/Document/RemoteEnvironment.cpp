#include "RemoteEnvironment.hpp"

#include <score/serialization/JSONVisitor.hpp>

#include <QObject>

#include <Network/Communication/Rpc.hpp>
#include <Network/Communication/WireJson.hpp>

namespace Network
{
namespace
{
QByteArray uriParams(const score::Uri& uri)
{
  const auto text = uri.toString().toUtf8();
  rapidjson::StringBuffer buf;
  JsonWriter w{buf};
  w.StartObject();
  w.Key("uri");
  w.String(text.constData(), text.size());
  w.EndObject();
  return QByteArray{buf.GetString(), (int)buf.GetLength()};
}

void reportGone(const score::Environment::Callback<QString>& onFailed)
{
  if(onFailed)
    onFailed(QObject::tr("the other machine did not answer"));
}
}

RemoteEnvironment::RemoteEnvironment(RpcChannel& rpc, Id<Client> peer)
    : m_rpc{&rpc}
    , m_peer{std::move(peer)}
{
}

bool RemoteEnvironment::stillConnected(const Callback<Failure>& onFailed) const
{
  if(m_rpc)
    return true;
  if(onFailed)
    onFailed(QObject::tr("this document is no longer part of a session"));
  return false;
}

RemoteEnvironment::~RemoteEnvironment() = default;

QString RemoteEnvironment::resolve(const score::Uri&) const
{
  // There is no path here that leads to it.
  return {};
}

void RemoteEnvironment::list(
    const score::Uri& uri, Callback<std::vector<score::DirEntry>> onListed,
    Callback<Failure> onFailed)
{
  if(!stillConnected(onFailed))
    return;

  m_rpc->call(
      m_peer, "fs.list", uriParams(uri),
      [onListed = std::move(onListed)](const rapidjson::Value& result) {
    if(!onListed)
      return;

    std::vector<score::DirEntry> entries;
    if(result.IsArray())
    {
      for(const auto& e : result.GetArray())
      {
        const auto uri = wireString(e, "uri");
        const auto name = wireString(e, "name");
        if(!uri || !name)
          continue;

        score::DirEntry entry;
        entry.uri = score::Uri::parse(*uri);
        entry.name = *name;
        entry.directory = wireBool(e, "directory").value_or(false);
        entry.size = wireInt(e, "size").value_or(0);
        entries.push_back(std::move(entry));
      }
    }
    onListed(std::move(entries));
      },
      std::move(onFailed));
}

void RemoteEnvironment::read(
    const score::Uri& uri, Callback<QByteArray> onRead, Callback<Failure> onFailed)
{
  if(!stillConnected(onFailed))
    return;

  m_rpc->call(
      m_peer, "fs.read", uriParams(uri),
      [onRead = std::move(onRead), onFailed](const rapidjson::Value& result) {
    if(!result.IsObject() || !result.HasMember("data") || !result["data"].IsString())
    {
      reportGone(onFailed);
      return;
    }
    if(onRead)
      onRead(QByteArray::fromBase64(QByteArray{
          result["data"].GetString(), (int)result["data"].GetStringLength()}));
      },
      onFailed);
}

void RemoteEnvironment::write(
    const score::Uri& uri, QByteArray data, Done onWritten, Callback<Failure> onFailed)
{
  if(!stillConnected(onFailed))
    return;

  const auto text = uri.toString().toUtf8();
  const auto encoded = data.toBase64();

  rapidjson::StringBuffer buf;
  JsonWriter w{buf};
  w.StartObject();
  w.Key("uri");
  w.String(text.constData(), text.size());
  w.Key("data");
  w.String(encoded.constData(), encoded.size());
  w.EndObject();

  m_rpc->call(
      m_peer, "fs.write", QByteArray{buf.GetString(), (int)buf.GetLength()},
      [onWritten = std::move(onWritten)](const rapidjson::Value&) {
    if(onWritten)
      onWritten();
      },
      std::move(onFailed));
}
}
