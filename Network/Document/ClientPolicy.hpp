#pragma once
#include <Network/Session/ClientSession.hpp>

#include "DocumentPlugin.hpp"

namespace iscore {
class Document;
}  // namespace iscore

// MOVEME
namespace Network
{
class ClientNetworkPolicy : public NetworkPolicyInterface
{
    public:
        ClientNetworkPolicy(ClientSession* s,
                            const iscore::DocumentContext& c);

        ClientSession* session() const override
        { return m_session; }
        void play() override;

    private:
        ClientSession* m_session{};
        const iscore::DocumentContext& m_ctx;
};
}
