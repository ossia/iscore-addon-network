#pragma once
#include <Network/Session/ClientSession.hpp>
#include <Network/Session/ClientSessionBuilder.hpp>
#include <Network/Document/Timekeeper.hpp>

#include "DocumentPlugin.hpp"

namespace iscore {
class Document;
}  // namespace iscore

// MOVEME
namespace Network
{
class ClientNetworkPolicy : public NetworkPolicy
{
    public:
        ClientNetworkPolicy(ClientSession* s,
                            const iscore::DocumentContext& c);

        ClientSession* session() const override
        { return m_session; }
        void play() override;

    private:
        void connectToOtherClient(QString ip, int port);
        ClientSession* m_session{};
        const iscore::DocumentContext& m_ctx;
        Timekeeper m_keep{*m_session};

        std::vector<std::unique_ptr<ClientSessionBuilder>> m_connections;
};
}
