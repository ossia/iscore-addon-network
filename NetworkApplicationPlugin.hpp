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


namespace Network
{
#ifdef USE_ZEROCONF
class ZeroconfBrowser;
#endif

class ClientSession;
class ClientSessionBuilder;
class NetworkApplicationPlugin : public QObject, public iscore::GUIApplicationContextPlugin
{
        Q_OBJECT

    public:
        NetworkApplicationPlugin(const iscore::ApplicationContext& app);
        void populateMenus(iscore::MenubarManager*) override;

    public slots:
        void setupClientConnection(QString ip, int port);

    private:

        std::unique_ptr<ClientSessionBuilder> m_sessionBuilder;

#ifdef USE_ZEROCONF
        ZeroconfBrowser* m_zeroconfBrowser{};
#endif
};
}
