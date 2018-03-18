#pragma once
#include <Network/Communication/NetworkMessage.hpp>
#include <Network/Communication/MessageMapper.hpp>
#include <Network/Communication/MessageValidator.hpp>
#include <QDataStream>
#include <QIODevice>
#include <QList>
#include <QString>
#include <algorithm>

#include <Network/Client/LocalClient.hpp>
#include <Network/Client/RemoteClient.hpp>
#include <score/model/IdentifiedObject.hpp>
#include <score/model/Identifier.hpp>

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
        void broadcastToAllClients(const NetworkMessage& m);

        //! Includes self
        void broadcastToAll(const NetworkMessage& m);
        void broadcastToOthers(const Id<Client>& sender, const NetworkMessage& m);

        void sendMessage(const Id<Client>& target, const NetworkMessage& m);
        void broadcastToClients(const std::vector<Id<Client> >& clts, const NetworkMessage& m);

    Q_SIGNALS:
        void clientAdded(RemoteClient*);
        void clientRemoving(RemoteClient*);
        void clientRemoved(RemoteClient*);
        void clientsChanged();

        void emitMessage(Id<Client> target, const NetworkMessage& m);

    public Q_SLOTS:
        void validateMessage(const NetworkMessage& m);

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
        mutable MessageMapper m_mapper;
        mutable MessageValidator m_validator;
        QList<RemoteClient*> m_remoteClients;
};
}
