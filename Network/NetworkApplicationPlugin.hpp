#pragma once
#include <iscore/plugins/application/GUIApplicationPlugin.hpp>
#include <QString>

#include <iscore/plugins/documentdelegate/plugin/DocumentPlugin.hpp>
#include <memory>
//#include <zmq.hpp>
namespace iscore {
class MenubarManager;
}  // namespace iscore
struct VisitorVariant;

#ifdef OSSIA_DNSSD
class ZeroconfBrowser;
#endif

namespace Network
{

class ClientSession;
class ClientSessionBuilder;
class NetworkApplicationPlugin :
        public QObject,
        public iscore::GUIApplicationPlugin
{
        Q_OBJECT

    public:
        NetworkApplicationPlugin(const iscore::GUIApplicationContext& app);

//        zmq::context_t zmq{1};
    public slots:
        void setupClientConnection(QString name, QString ip, int port, QMap<QString, QByteArray>);
        void setupPlayerConnection(QString name, QString ip, int port, QMap<QString, QByteArray>);

    private:
        GUIElements makeGUIElements() override;
        std::unique_ptr<ClientSessionBuilder> m_sessionBuilder;

#ifdef OSSIA_DNSSD
        ZeroconfBrowser* m_serverBrowser{};
        ZeroconfBrowser* m_playerBrowser{};
#endif
};
}
