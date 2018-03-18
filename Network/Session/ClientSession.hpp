#pragma once
#include <Network/Session/Session.hpp>
#include <score/model/Identifier.hpp>

namespace Network
{
class LocalClient;
class RemoteClient;
class ClientSession final : public Session
{
    public:
        ClientSession(
                RemoteClient& master,
                LocalClient* client,
                Id<Session> id,
                QObject* parent = nullptr);

        RemoteClient& master() const override
        {
            return m_master;
        }


    private:
        void on_createNewClient(QWebSocket*);
        RemoteClient& m_master;
};
}
