#include "Rpc.hpp"

#include <score/serialization/JSONVisitor.hpp>

#include <QDebug>
#include <QTimer>

#include <Network/Communication/MessageMapper.hpp>
#include <Network/Communication/WireRead.hpp>

#include <rapidjson/reader.h>
#include <Network/Communication/NetworkMessage.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <Network/Session/Session.hpp>

#include <exception>
#include <utility>
#include <vector>

namespace Network
{
namespace
{
//! Whether a body a caller handed us is JSON we can put in an envelope as it
//! stands. Callers build these with JsonWriter, so it is; a corrupt one becomes
//! a null rather than corrupting the envelope around it.
//! Checked without building a document: the answer is one bit, and a DOM for a
//! file read near the inline limit is megabytes allocated to learn it.
bool wellFormed(const QByteArray& json)
{
  rapidjson::StringStream stream{json.constData()};
  rapidjson::BaseReaderHandler<> ignore;
  rapidjson::Reader reader;
  return !reader.Parse<rapidjson::kParseStopWhenDoneFlag>(stream, ignore).IsError();
}

//! Spliced, not re-encoded.
//!
//! These bodies are already JSON: parsing them into a DOM and writing them out
//! again produced identical bytes at the cost of a full parse and two more
//! buffers the size of the payload. For a file read near the inline limit that
//! is ~10 MB parsed and copied twice for nothing, on a browser's one thread.
QByteArray requestBody(int64_t id, const QByteArray& method, const QByteArray& params)
{
  const bool usable = !params.isEmpty() && wellFormed(params);

  QByteArray out;
  out.reserve(params.size() + method.size() + 64);
  out += "{\"id\":";
  out += QByteArray::number((qlonglong)id);
  out += ",\"method\":";
  {
    rapidjson::StringBuffer buf;
    JsonWriter w{buf};
    w.String(method.constData(), method.size());
    out += QByteArray{buf.GetString(), (int)buf.GetLength()};
  }
  out += ",\"params\":";
  out += usable ? params : QByteArrayLiteral("null");
  out += "}";
  return out;
}

QByteArray resultBody(int64_t id, const QByteArray& result)
{
  const bool usable = !result.isEmpty() && wellFormed(result);

  QByteArray out;
  out.reserve(result.size() + 64);
  out += "{\"id\":";
  out += QByteArray::number((qlonglong)id);
  out += ",\"result\":";
  out += usable ? result : QByteArrayLiteral("null");
  out += "}";
  return out;
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
  // Anything still waiting will never be answered now. Taken first, since a
  // callback may start another request and would otherwise be adding to the
  // container being walked.
  auto pending = std::exchange(m_pending, {});
  for(auto& [id, p] : pending)
  {
    if(p.onError)
      p.onError(QObject::tr("the session ended"));
  }
}

void RpcChannel::resolve(
    int64_t id, const rapidjson::Value* result, const QString& error)
{
  auto it = m_pending.find(id);
  if(it == m_pending.end())
    return;

  const auto pending = it->second;
  m_pending.erase(it);

  if(result)
  {
    // An answer is a peer's bytes, and what a caller does with one is
    // deserialize it -- through factories written for our own save files, which
    // assert their way through anything else. Guarded here rather than at each
    // call site, so a caller cannot forget. onRequest already does this for the
    // asking direction.
    if(pending.onResult)
    {
      readingWireData("an answer to a request", [&] { pending.onResult(*result); });
    }
  }
  else if(pending.onError)
  {
    pending.onError(error);
  }
}

void RpcChannel::peerLost(const Id<Client>& peer)
{
  std::vector<int64_t> lost;
  for(const auto& [id, pending] : m_pending)
  {
    if(pending.peer == peer)
      lost.push_back(id);
  }
  for(auto id : lost)
    resolve(id, nullptr, QObject::tr("that machine left the session"));
}

void RpcChannel::bind(QByteArray method, Handler handler)
{
  m_handlers[std::move(method)] = std::move(handler);
}

void RpcChannel::send(const Id<Client>& target, const NetworkMessage& m)
{
  // ClientSession adds its master to the remote clients, so sendMessage finds
  // it like any other peer. Asking master() first was both unnecessary and
  // fatal on a session that has none: the base returns by throwing.
  m_session.sendMessage(target, m);
}

void RpcChannel::call(
    const Id<Client>& peer, QByteArray method, const QByteArray& params,
    OnResult onResult, OnError onError, int timeoutMs)
{
  const auto id = m_nextId++;
  m_pending[id] = Pending{peer, std::move(onResult), std::move(onError)};

  auto& mapi = MessagesAPI::instance();
  NetworkMessage m;
  m.address = mapi.rpc_request;
  m.clientId = m_session.localClient().id();
  m.sessionId = m_session.id();
  m.data = requestBody(id, method, params);
  send(peer, m);

  // Session::sendMessage drops a message whose target is gone, and a peer may
  // simply never answer. Without this the caller waits forever and neither
  // callback ever runs.
  if(timeoutMs > 0)
  {
    QTimer::singleShot(timeoutMs, this, [this, id] {
      resolve(id, nullptr, QObject::tr("that machine did not answer"));
    });
  }
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

  if(!doc["id"].IsInt64() || !doc["method"].IsString())
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
  if(doc.HasParseError() || !doc.IsObject() || !doc.HasMember("id")
     || !doc["id"].IsInt64())
    return;

  const auto it = m_pending.find(doc["id"].GetInt64());
  if(it == m_pending.end())
    return;

  // From whoever was asked, and nobody else.
  if(!(it->second.peer == m.clientId))
  {
    qWarning() << "Ignoring an answer from a peer that was not asked";
    return;
  }

  const auto id = it->first;
  if(doc.HasMember("error"))
  {
    resolve(
        id, nullptr,
        doc["error"].IsString()
            ? QString::fromUtf8(
                  doc["error"].GetString(), doc["error"].GetStringLength())
            : QObject::tr("the request failed"));
    return;
  }

  static const rapidjson::Value nullResult;
  resolve(id, doc.HasMember("result") ? &doc["result"] : &nullResult, {});
}
}
