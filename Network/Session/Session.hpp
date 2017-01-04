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

class QObject;


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


        MessageValidator& validator()
        {
            return *m_validator;
        }
        MessageMapper& mapper()
        {
            return *m_mapper;
        }

        LocalClient& localClient()
        {
            return *m_client;
        }

        const LocalClient& localClient() const
        {
            return *m_client;
        }

        const QList<RemoteClient*>& remoteClients() const
        {
            return m_remoteClients;
        }

        RemoteClient* findClient(Id<Client> target)
        {
          const auto& c = remoteClients();
          auto it = ossia::find(c, target);
          if(it != c.end())
            return *it;
          return nullptr;
        }

        void addClient(RemoteClient* clt)
        {
            clt->setParent(this);
            m_remoteClients.append(clt);
            emit clientAdded(clt);
            emit clientsChanged();
        }

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

        void broadcastToAllClients(NetworkMessage m)
        {
            for(RemoteClient* client : remoteClients())
                client->sendMessage(m);
        }

        void broadcastToOthers(Id<Client> sender, NetworkMessage m)
        {
            for(const auto& client : remoteClients())
            {
                if(client->id() != sender)
                    client->sendMessage(m);
            }
        }

        void sendMessage(Id<Client> target, NetworkMessage m)
        {
          const auto& c = remoteClients();
          auto it = ossia::find(c, target);
          if(it != c.end())
            (*it)->sendMessage(m);
        }

    signals:
        void clientAdded(RemoteClient*);
        void clientsChanged();

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
