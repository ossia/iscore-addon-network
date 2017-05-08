#pragma once
#include <Network/Session/Session.hpp>
#include <iscore/model/Identifier.hpp>
#include <QList>


class QObject;
class QWebSocket;

namespace servus { class Servus; }
namespace iscore {
class Document;
}
namespace Network
{
class Client;
class LocalClient;
class RemoteClient;
class RemoteClientBuilder;
struct NetworkMessage;
class MasterSession : public Session
{
           Q_OBJECT
    public:
        MasterSession(iscore::Document* doc,
                      LocalClient* theclient,
                      Id<Session> id,
                      QObject* parent = nullptr);
        ~MasterSession();

        iscore::Document* document() const
        { return m_document; }

        LocalClient& master() const override
        { return this->localClient(); }

    public slots:
        void on_createNewClient(QWebSocket* sock);
        void on_clientReady(RemoteClientBuilder* bldr, RemoteClient* clt);

    private:
        iscore::Document* m_document{};
        QList<RemoteClientBuilder*> m_clientBuilders;

#ifdef OSSIA_DNSSD
        std::unique_ptr<servus::Servus> m_dnssd;
#endif

};

}
