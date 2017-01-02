#pragma once
#include <Network/Session/MasterSession.hpp>

#include "DocumentPlugin.hpp"

namespace iscore {
struct DocumentContext;
}  // namespace iscore

namespace Network
{

class MasterNetworkPolicy : public NetworkPolicyInterface
{
    public:
        MasterNetworkPolicy(MasterSession* s,
                            const iscore::DocumentContext& c);

        MasterSession* session() const override
        { return m_session; }
        void play() override;

    private:
        MasterSession* m_session{};
        const iscore::DocumentContext& m_ctx;
};
}
