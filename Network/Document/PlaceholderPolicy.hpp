#pragma once
#include "DocumentPlugin.hpp"

class QObject;

namespace Network
{
class Session;
class PlaceholderNetworkPolicy : public NetworkPolicy
{
    public:
        template<typename Deserializer>
        PlaceholderNetworkPolicy(Deserializer&& vis, QObject* parent) :
            NetworkPolicy{parent}
        {
            vis.writeTo(*this);
        }

        Session* session() const override
        { return m_session; }

        void play() override { }
        Session* m_session{};
};
}

