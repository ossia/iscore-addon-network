#pragma once
#include <score/plugins/application/GUIApplicationPlugin.hpp>
#include <wobjectdefs.h>
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
        W_OBJECT(NetworkApplicationPlugin)

    public:
        NetworkApplicationPlugin(const score::GUIApplicationContext& app);
        ~NetworkApplicationPlugin();

        void setupClientConnection(QString name, QString ip, int port, QMap<QString, QByteArray>);
        W_SLOT(setupClientConnection)
        void setupPlayerConnection(QString name, QString ip, int port, QMap<QString, QByteArray>);
        W_SLOT(setupPlayerConnection)

    private:
        GUIElements makeGUIElements() override;
        std::unique_ptr<ClientSessionBuilder> m_sessionBuilder;

#if defined(OSSIA_DNSSD)
        ZeroconfBrowser* m_serverBrowser{};
        ZeroconfBrowser* m_playerBrowser{};
#endif
};
}

using string_ba_map = QMap<QString, QByteArray>;
W_REGISTER_ARGTYPE(string_ba_map)
