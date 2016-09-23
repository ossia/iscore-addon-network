#pragma once
#include <iscore/plugins/application/GUIApplicationContextPlugin.hpp>
#include <QString>

#include <iscore/plugins/documentdelegate/plugin/DocumentDelegatePluginModel.hpp>
#include <memory>

namespace iscore {

class Document;
class MenubarManager;

}  // namespace iscore
struct VisitorVariant;

#ifdef USE_ZEROCONF
class ZeroconfBrowser;
#endif

namespace Network
{

class ClientSession;
class ClientSessionBuilder;
class NetworkApplicationPlugin :
        public QObject,
        public iscore::GUIApplicationContextPlugin
{
        Q_OBJECT

    public:
        NetworkApplicationPlugin(const iscore::GUIApplicationContext& app);

    public slots:
        void setupClientConnection(QString ip, int port);

    private:
        GUIElements makeGUIElements() override;
        std::unique_ptr<ClientSessionBuilder> m_sessionBuilder;

#ifdef USE_ZEROCONF
        ZeroconfBrowser* m_zeroconfBrowser{};
#endif
};
}
