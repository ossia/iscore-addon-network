#pragma once
#include <score/plugins/application/GUIApplicationPlugin.hpp>

#include <Network/Client/PeerRole.hpp>

#include <memory>
#include <verdigris>

#ifdef OSSIA_DNSSD
class ZeroconfBrowser;
#endif

namespace Network
{
class ClientSession;
class ClientSessionBuilder;
class NetworkApplicationPlugin
    : public QObject
    , public score::GUIApplicationPlugin
{
  W_OBJECT(NetworkApplicationPlugin)

public:
  NetworkApplicationPlugin(const score::GUIApplicationContext& app);
  ~NetworkApplicationPlugin();

  void on_createdDocument(score::Document& doc) override;
  bool handleLoading() override;

  //! Kept at four arguments: it is connected to ZeroconfBrowser by member
  //! pointer, where a defaulted parameter would not count.
  void
  setupClientConnection(QString name, QString ip, int port, QMap<QString, QByteArray>);
  W_SLOT(setupClientConnection)
  void joinSession(QString ip, int port, PeerRole role);
  void
  setupPlayerConnection(QString name, QString ip, int port, QMap<QString, QByteArray>);
  W_SLOT(setupPlayerConnection)

private:
  void do_makeServer(score::Document& doc);
  GUIElements makeGUIElements() override;
  std::unique_ptr<ClientSessionBuilder> m_sessionBuilder;

  QString m_arg_net_join;
  QString m_arg_net_host;
  PeerRole m_arg_role{PeerRole::Performer};

#if defined(OSSIA_DNSSD)
  ZeroconfBrowser* m_serverBrowser{};
  ZeroconfBrowser* m_playerBrowser{};
#endif
};
}

using string_ba_map = QMap<QString, QByteArray>;
W_REGISTER_ARGTYPE(string_ba_map)
