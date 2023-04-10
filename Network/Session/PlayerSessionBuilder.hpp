#pragma once
#include <score/command/Command.hpp>
#include <score/command/CommandData.hpp>
#include <score/model/Identifier.hpp>
#include <score/tools/std/Optional.hpp>

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QPair>
#include <QString>

#include <functional>
#include <verdigris>

namespace score
{
struct GUIApplicationContext;
class Document;

}

namespace Network
{
class Client;
class ClientSession;
class NetworkSocket;
class Session;
struct NetworkMessage;

class PlayerSessionBuilder final : public QObject
{
  W_OBJECT(PlayerSessionBuilder)
public:
  PlayerSessionBuilder(const score::ApplicationContext&, QString ip, int port);

  std::function<score::Document*(const QByteArray&)> documentLoader;

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
  const score::ApplicationContext& m_context;
  QString m_clientName{"A Client"};
  Id<Client> m_masterId, m_clientId;
  Id<Session> m_sessionId;
  NetworkSocket* m_mastersocket{};

  std::vector<score::CommandData> m_commandStack;
  QByteArray m_documentData;

  ClientSession* m_session{};
};
}
