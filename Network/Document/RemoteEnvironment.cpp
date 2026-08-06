#include "RemoteEnvironment.hpp"

#include <score/serialization/JSONVisitor.hpp>

#include <QObject>

#include <Network/Communication/Rpc.hpp>

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
    : m_rpc{rpc}
    , m_peer{std::move(peer)}
{
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
  m_rpc.call(
      m_peer, "fs.list", uriParams(uri),
      [onListed = std::move(onListed)](const rapidjson::Value& result) {
    if(!onListed)
      return;

    std::vector<score::DirEntry> entries;
    if(result.IsArray())
    {
      for(const auto& e : result.GetArray())
      {
        if(!e.IsObject() || !e.HasMember("uri") || !e.HasMember("name"))
          continue;

        score::DirEntry entry;
        entry.uri = score::Uri::parse(QString::fromUtf8(
            e["uri"].GetString(), e["uri"].GetStringLength()));
        entry.name = QString::fromUtf8(
            e["name"].GetString(), e["name"].GetStringLength());
        entry.directory = e.HasMember("directory") && e["directory"].GetBool();
        entry.size = e.HasMember("size") ? e["size"].GetInt64() : 0;
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
  m_rpc.call(
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

  m_rpc.call(
      m_peer, "fs.write", QByteArray{buf.GetString(), (int)buf.GetLength()},
      [onWritten = std::move(onWritten)](const rapidjson::Value&) {
    if(onWritten)
      onWritten();
      },
      std::move(onFailed));
}
}
