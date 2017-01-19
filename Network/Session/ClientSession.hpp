#pragma once
#include "Session.hpp"

class QObject;
#include <iscore/model/Identifier.hpp>

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
        RemoteClient& m_master;
};
}
