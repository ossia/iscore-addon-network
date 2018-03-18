#pragma once
#include <score/plugins/application/GUIApplicationPlugin.hpp>
#include <memory>

#ifdef OSSIA_DNSSD
class ZeroconfBrowser;
#endif

namespace Network
{
class ClientSession;
class ClientSessionBuilder;
class NetworkApplicationPlugin :
        public QObject,
        public score::GUIApplicationPlugin
{
        Q_OBJECT

    public:
        NetworkApplicationPlugin(const score::GUIApplicationContext& app);
        ~NetworkApplicationPlugin();

    public Q_SLOTS:
        void setupClientConnection(QString name, QString ip, int port, QMap<QString, QByteArray>);
        void setupPlayerConnection(QString name, QString ip, int port, QMap<QString, QByteArray>);

    private:
        GUIElements makeGUIElements() override;
        std::unique_ptr<ClientSessionBuilder> m_sessionBuilder;

#if defined(OSSIA_DNSSD)
        ZeroconfBrowser* m_serverBrowser{};
        ZeroconfBrowser* m_playerBrowser{};
#endif
};
}
