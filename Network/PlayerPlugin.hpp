#pragma once
#include <iscore/plugins/application/GUIApplicationPlugin.hpp>
#include <QTcpServer>
#include <functional>
#include <iscore_addon_network_export.h>
namespace servus { class Servus; }
namespace Network
{
class PlayerSessionBuilder;
class ISCORE_ADDON_NETWORK_EXPORT PlayerPlugin
    : public QObject
    , public iscore::ApplicationPlugin
{
public:
  PlayerPlugin(const iscore::ApplicationContext& ctx);
  ~PlayerPlugin();

  void initialize() override;

  std::function<iscore::Document*(const QByteArray&)> documentLoader;
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
