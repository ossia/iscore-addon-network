#pragma once
#include <Network/Session/MasterSession.hpp>

#include "DocumentPlugin.hpp"
#include <Network/Document/Timekeeper.hpp>

namespace iscore {
struct DocumentContext;
}  // namespace iscore

namespace Network
{

class MasterEditionPolicy : public EditionPolicy
{
    public:
        MasterEditionPolicy(MasterSession* s,
                            const iscore::DocumentContext& c);

        MasterSession* session() const override
        { return m_session; }
        void play() override;

    private:
        MasterSession* m_session{};
        const iscore::DocumentContext& m_ctx;
        Timekeeper m_keep{*m_session};
};
}