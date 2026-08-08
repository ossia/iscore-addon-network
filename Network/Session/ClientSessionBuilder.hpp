#pragma once
#include <Network/Client/PeerRole.hpp>

#include <memory>
#include <Network/Communication/Capabilities.hpp>
#include <score_addon_network_export.h>
#include <score/command/Command.hpp>
#include <score/command/CommandData.hpp>
#include <score/model/Identifier.hpp>
#include <score/tools/std/Optional.hpp>

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QUrl>
#include <QPair>
#include <QString>

#include <verdigris>
namespace score
{
struct GUIApplicationContext;
}

namespace Network
{
class Client;
class ClientSession;
class NetworkSocket;
class Session;
class RemoteDeviceCatalog;
struct NetworkMessage;

//! Used by a client to join a Session.
class SCORE_ADDON_NETWORK_EXPORT ClientSessionBuilder final : public QObject
{
  W_OBJECT(ClientSessionBuilder)
public:
  ClientSessionBuilder(
      const score::GUIApplicationContext&, QString ip, int port,
      PeerRole role = PeerRole::Performer);

  void initiateConnection();
  ClientSession* builtSession() const;

  //! What the host agreed to, which is what the document was built for.
  PeerRole role() const noexcept { return m_role; }

  //! What the host can construct. Differs from ours whenever the two builds do.
  const Capabilities& masterCapabilities() const noexcept
  {
    return m_masterCapabilities;
  }
  QByteArray documentData() const;
  const std::vector<score::CommandData>& commandStackData() const;

  void on_messageReceived(const NetworkMessage& m);

  //! Load the received document, off the socket callback.
  void buildDocument();
  W_SLOT(on_messageReceived)

  void connected() W_SIGNAL(connected);
  void sessionReady() W_SIGNAL(sessionReady);
  void sessionFailed() W_SIGNAL(sessionFailed);

  //! The host was never reached. `url` is what was tried.
  void connectionFailed(QUrl url, QString reason)
      W_SIGNAL(connectionFailed, url, reason)

private:
  const score::GUIApplicationContext& m_context;
  QString m_clientName{"A Client"};
  Id<Client> m_masterId, m_clientId;
  Id<Session> m_sessionId;
  NetworkSocket* m_mastersocket{};

  std::vector<score::CommandData> m_commandStack;
  QByteArray m_documentData;
  QByteArray m_sessionMessage;

  ClientSession* m_session{};
  Capabilities m_masterCapabilities;
  PeerRole m_role{PeerRole::Performer};

};
}
