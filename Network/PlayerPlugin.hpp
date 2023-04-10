#pragma once
#include <score/plugins/application/GUIApplicationPlugin.hpp>

#include <QTcpServer>

#include <score_addon_network_export.h>

#include <functional>

#undef OSSIA_DNSSD
namespace servus
{
class Servus;
}
namespace Network
{
class PlayerSessionBuilder;
class SCORE_ADDON_NETWORK_EXPORT PlayerPlugin
    : public QObject
    , public score::ApplicationPlugin
{
public:
  PlayerPlugin(const score::ApplicationContext& ctx);
  ~PlayerPlugin();

  void initialize() override;

  std::function<score::Document*(const QByteArray&)> documentLoader;
  std::function<void()> onDocumentLoaded;

private:
  void setupServer();
  QTcpServer m_listenServer;
  std::unique_ptr<PlayerSessionBuilder> m_sessionBuilder;

#if defined(OSSIA_DNSSD)
  std::unique_ptr<servus::Servus> m_zeroconf;
#endif
};
}
