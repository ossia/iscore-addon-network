#pragma once
#include <score/command/Command.hpp>
#include <score/command/CommandData.hpp>
#include <score/model/Identifier.hpp>
#include <score/tools/std/Optional.hpp>

#include <verdigris>

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
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
struct NetworkMessage;

//! Used by a client to join a Session.
class ClientSessionBuilder final : public QObject
{
  W_OBJECT(ClientSessionBuilder)
public:
  ClientSessionBuilder(
      const score::GUIApplicationContext&,
      QString ip,
      int port);

  void initiateConnection();
  ClientSession* builtSession() const;
  QByteArray documentData() const;
  const std::vector<score::CommandData>& commandStackData() const;

  void on_messageReceived(const NetworkMessage& m);
  W_SLOT(on_messageReceived)

  void connected() W_SIGNAL(connected);
  void sessionReady() W_SIGNAL(sessionReady);
  void sessionFailed() W_SIGNAL(sessionFailed);

private:
  const score::GUIApplicationContext& m_context;
  QString m_clientName{"A Client"};
  Id<Client> m_masterId, m_clientId;
  Id<Session> m_sessionId;
  NetworkSocket* m_mastersocket{};

  std::vector<score::CommandData> m_commandStack;
  QByteArray m_documentData;

  ClientSession* m_session{};
};
}
