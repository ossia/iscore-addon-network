#pragma once
#include <Network/Communication/NetworkMessage.hpp>
#include <QDataStream>
#include <QIODevice>
#include <QList>
#include <QString>
#include <algorithm>

#include <Network/Client/LocalClient.hpp>
#include <Network/Client/RemoteClient.hpp>
#include <iscore/model/IdentifiedObject.hpp>
#include <iscore/model/Identifier.hpp>

namespace Network
{
class MessageMapper;
class MessageValidator;
class Session : public IdentifiedObject<Session>
{
        Q_OBJECT
    public:
        Session(LocalClient* client,
                Id<Session> id,
                QObject* parent = nullptr);
        ~Session();

        MessageValidator& validator() const;
        MessageMapper& mapper() const;

        virtual Client& master() const;
        LocalClient& localClient() const;

        const QList<RemoteClient*>& remoteClients() const;
        RemoteClient* findClient(Id<Client> target);
        void addClient(RemoteClient* clt);
        void removeClient(RemoteClient* clt);

        NetworkMessage makeMessage(const QByteArray& address);

        template<typename... Args>
        NetworkMessage makeMessage(const QByteArray& address, Args&&... args)
        {
            NetworkMessage m;
            m.address = address;
            m.clientId = localClient().id();
            m.sessionId = id();

            impl_makeMessage(QDataStream{&m.data, QIODevice::WriteOnly}, std::forward<Args&&>(args)...);

            return m;
        }

        //! Does not include self
        void broadcastToAllClients(NetworkMessage m);

        //! Includes self
        void broadcastToAll(NetworkMessage m);
        void broadcastToOthers(Id<Client> sender, NetworkMessage m);

        void sendMessage(Id<Client> target, NetworkMessage m);
        void broadcastToClients(const std::vector<Id<Client> >& clts, NetworkMessage m);

    signals:
        void clientAdded(RemoteClient*);
        void clientRemoved(RemoteClient*);
        void clientsChanged();

        void emitMessage(Id<Client> target, NetworkMessage m);

    public slots:
        void validateMessage(NetworkMessage m);

    private:
        template<typename Arg>
        void impl_makeMessage(QDataStream&& s, Arg&& arg)
        {
            s << arg;
        }

        template<typename Arg, typename... Args>
        void impl_makeMessage(QDataStream&& s, Arg&& arg, Args&&... args)
        {
            impl_makeMessage(std::move(s << arg), std::forward<Args&&>(args)...);
        }

        LocalClient* m_client{};
        MessageMapper* m_mapper{};
        MessageValidator* m_validator{};
        QList<RemoteClient*> m_remoteClients;
};
}
