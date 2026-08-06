#include "Rpc.hpp"

#include <score/serialization/JSONVisitor.hpp>

#include <QDebug>

#include <Network/Client/RemoteClient.hpp>
#include <Network/Communication/MessageMapper.hpp>
#include <Network/Communication/NetworkMessage.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <Network/Session/Session.hpp>

#include <exception>

namespace Network
{
namespace
{
QByteArray requestBody(int64_t id, const QByteArray& method, const QByteArray& params)
{
  rapidjson::StringBuffer buf;
  JsonWriter w{buf};
  w.StartObject();
  w.Key("id");
  w.Int64(id);
  w.Key("method");
  w.String(method.constData(), method.size());
  w.Key("params");
  if(params.isEmpty())
  {
    w.Null();
  }
  else
  {
    rapidjson::Document d;
    d.Parse(params.constData(), params.size());
    if(d.HasParseError())
      w.Null();
    else
      d.Accept(w);
  }
  w.EndObject();
  return QByteArray{buf.GetString(), (int)buf.GetLength()};
}

QByteArray resultBody(int64_t id, const QByteArray& result)
{
  rapidjson::StringBuffer buf;
  JsonWriter w{buf};
  w.StartObject();
  w.Key("id");
  w.Int64(id);
  w.Key("result");
  rapidjson::Document d;
  d.Parse(result.constData(), result.size());
  if(result.isEmpty() || d.HasParseError())
    w.Null();
  else
    d.Accept(w);
  w.EndObject();
  return QByteArray{buf.GetString(), (int)buf.GetLength()};
}

QByteArray errorBody(int64_t id, const QString& message)
{
  const auto utf8 = message.toUtf8();
  rapidjson::StringBuffer buf;
  JsonWriter w{buf};
  w.StartObject();
  w.Key("id");
  w.Int64(id);
  w.Key("error");
  w.String(utf8.constData(), utf8.size());
  w.EndObject();
  return QByteArray{buf.GetString(), (int)buf.GetLength()};
}
}

RpcChannel::RpcChannel(Session& session)
    : m_session{session}
{
  auto& mapi = MessagesAPI::instance();
  session.mapper().addHandler(this, 
      mapi.rpc_request, [this](const NetworkMessage& m) { onRequest(m); });
  session.mapper().addHandler(this, 
      mapi.rpc_response, [this](const NetworkMessage& m) { onResponse(m); });
}

RpcChannel::~RpcChannel()
{
  // Anything still waiting will never be answered now.
  for(auto& [id, pending] : m_pending)
  {
    if(pending.onError)
      pending.onError(QObject::tr("the session ended"));
  }
}

void RpcChannel::bind(QByteArray method, Handler handler)
{
  m_handlers[std::move(method)] = std::move(handler);
}

void RpcChannel::send(const Id<Client>& target, const NetworkMessage& m)
{
  // A client reaches its one peer, the master, directly. Session::sendMessage
  // looks through the remote clients, which on a client is not where the master
  // is kept. On a master, master() is the local client and this does not apply.
  if(target == m_session.master().id())
  {
    if(auto* remote = dynamic_cast<RemoteClient*>(&m_session.master()))
    {
      remote->sendMessage(m);
      return;
    }
  }
  m_session.sendMessage(target, m);
}

void RpcChannel::call(
    const Id<Client>& peer, QByteArray method, const QByteArray& params,
    OnResult onResult, OnError onError)
{
  const auto id = m_nextId++;
  m_pending[id] = Pending{std::move(onResult), std::move(onError)};

  auto& mapi = MessagesAPI::instance();
  NetworkMessage m;
  m.address = mapi.rpc_request;
  m.clientId = m_session.localClient().id();
  m.sessionId = m_session.id();
  m.data = requestBody(id, method, params);
  send(peer, m);
}

void RpcChannel::onRequest(const NetworkMessage& m)
{
  auto doc = readJson(m.data);
  if(doc.HasParseError() || !doc.IsObject() || !doc.HasMember("id")
     || !doc.HasMember("method"))
  {
    qWarning() << "Ignoring a malformed request";
    return;
  }

  const int64_t id = doc["id"].GetInt64();
  const QByteArray method{doc["method"].GetString(), (int)doc["method"].GetStringLength()};

  auto& mapi = MessagesAPI::instance();
  NetworkMessage reply;
  reply.address = mapi.rpc_response;
  reply.clientId = m_session.localClient().id();
  reply.sessionId = m_session.id();

  auto it = m_handlers.find(method);
  if(it == m_handlers.end())
  {
    // Peers do not all offer the same methods, for the same reason they do not
    // all have the same plug-ins. Saying so is the answer, not a failure.
    reply.data = errorBody(id, QObject::tr("no such method: %1").arg(QString{method}));
  }
  else
  {
    static const rapidjson::Value nullParams;
    const auto& params = doc.HasMember("params") ? doc["params"] : nullParams;
    try
    {
      reply.data = resultBody(id, it->second(params));
    }
    catch(const std::exception& e)
    {
      reply.data = errorBody(id, QString::fromUtf8(e.what()));
    }
    catch(...)
    {
      reply.data = errorBody(id, QObject::tr("the request could not be answered"));
    }
  }

  send(m.clientId, reply);
}

void RpcChannel::onResponse(const NetworkMessage& m)
{
  auto doc = readJson(m.data);
  if(doc.HasParseError() || !doc.IsObject() || !doc.HasMember("id"))
    return;

  const auto it = m_pending.find(doc["id"].GetInt64());
  if(it == m_pending.end())
    return;

  const auto pending = it->second;
  m_pending.erase(it);

  if(doc.HasMember("error"))
  {
    if(pending.onError)
      pending.onError(QString::fromUtf8(
          doc["error"].GetString(), doc["error"].GetStringLength()));
  }
  else if(pending.onResult)
  {
    static const rapidjson::Value nullResult;
    pending.onResult(doc.HasMember("result") ? doc["result"] : nullResult);
  }
}
}
