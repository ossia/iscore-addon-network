#pragma once
#include "DocumentPlugin.hpp"

class QObject;

namespace Network
{
class Session;
class PlaceholderEditionPolicy : public EditionPolicy
{
    public:
        template<typename Deserializer>
        PlaceholderEditionPolicy(Deserializer&& vis, QObject* parent) :
            EditionPolicy{parent}
        {
            vis.writeTo(*this);
        }

        Session* session() const override
        { return m_session; }

        void play() override { }
        Session* m_session{};
};
}

