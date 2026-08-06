#pragma once
#include <score/model/Identifier.hpp>

#include <ossia/detail/hash_map.hpp>

#include <QByteArray>
#include <QObject>

#include <score_addon_network_export.h>

#include <functional>

#include <ossia/detail/json_fwd.hpp>

namespace Network
{
class Client;
class Session;
struct NetworkMessage;

/**
 * @brief Asking a peer something, and getting an answer.
 *
 * Command replication is the wrong shape for a question. It is one-way, it is
 * ordered against the document, and every message is a change everyone must
 * apply. "Which cameras do you have?" is none of those: it is addressed to one
 * peer, it expects a reply, and the answer changes nothing.
 *
 * So this runs alongside it on the same socket, as its own pair of message
 * addresses. Requests carry a number that the reply quotes back, which is what
 * lets several be in flight without the answers being confusable.
 *
 * Payloads are JSON rather than the QDataStream the document channel uses:
 * what travels here describes another machine's world -- protocols and devices
 * this build may have no type for -- and JSON can be carried through by
 * something that cannot parse it, which is exactly the case that matters.
 */
class SCORE_ADDON_NETWORK_EXPORT RpcChannel : public QObject
{
public:
  //! Answers a request. Receives the request's params; returns the result as
  //! JSON. Throwing turns into an error reply rather than taking the peer down.
  using Handler = std::function<QByteArray(const rapidjson::Value& params)>;

  using OnResult = std::function<void(const rapidjson::Value& result)>;
  using OnError = std::function<void(const QString& message)>;

  explicit RpcChannel(Session& session);
  ~RpcChannel();

  //! Offer `method` to peers.
  void bind(QByteArray method, Handler handler);

  //! Ask `peer`. `onError` is called if it refuses, cannot, or does not know
  //! the method.
  void call(
      const Id<Client>& peer, QByteArray method, const QByteArray& params,
      OnResult onResult, OnError onError = {});

private:
  void onRequest(const NetworkMessage& m);
  void onResponse(const NetworkMessage& m);
  void send(const Id<Client>& target, const NetworkMessage& m);

  struct Pending
  {
    OnResult onResult;
    OnError onError;
  };

  Session& m_session;
  ossia::hash_map<QByteArray, Handler> m_handlers;
  ossia::hash_map<int64_t, Pending> m_pending;
  int64_t m_nextId{1};
};
}
