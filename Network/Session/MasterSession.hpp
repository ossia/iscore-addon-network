#pragma once
#include <score/model/Identifier.hpp>

#include <QList>

#include <Network/Session/Session.hpp>

#undef OSSIA_DNSSD
class QObject;
class QWebSocket;

namespace servus
{
class Servus;
}
namespace score
{
class Document;
}
namespace Network
{
class Client;
class LocalClient;
class RemoteClient;
class RemoteClientBuilder;
struct NetworkMessage;
class MasterSession : public Session
{
  W_OBJECT(MasterSession)
public:
  MasterSession(
      const score::DocumentContext& doc,
      LocalClient* theclient,
      Id<Session> id,
      QObject* parent = nullptr);
  ~MasterSession();

  const score::DocumentContext& document() const { return m_ctx; }

  LocalClient& master() const override { return this->localClient(); }

  void on_createNewClient(QWebSocket* sock);
  W_SLOT(on_createNewClient);
  void on_clientReady(RemoteClientBuilder* bldr, RemoteClient* clt);
  W_SLOT(on_clientReady);

private:
  const score::DocumentContext& m_ctx;
  QList<RemoteClientBuilder*> m_clientBuilders;

#ifdef OSSIA_DNSSD
  std::unique_ptr<servus::Servus> m_dnssd;
#endif
};
}

W_REGISTER_ARGTYPE(Network::RemoteClientBuilder*)
