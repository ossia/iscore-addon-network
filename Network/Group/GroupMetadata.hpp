#pragma once

#include <score/model/ComponentSerialization.hpp>
#include <score/tools/std/Optional.hpp>
#include <QObject>

#include <QString>


#include <score/model/Identifier.hpp>

struct VisitorVariant;

namespace Network
{
class Group;
//! Goes into the intervals, events, etc.
class GroupMetadata :
    public score::SerializableComponent
{
        Q_OBJECT

    public:
        GroupMetadata(
                Id<Component> self,
                Id<Group> id,
                QObject* parent);

        ~GroupMetadata();

        template<typename DeserializerVisitor>
        GroupMetadata(DeserializerVisitor&& vis,
                      QObject* parent) :
            score::SerializableComponent{vis, parent}
        {
            vis.writeTo(*this);
        }

        const auto& group() const
        { return m_id; }

    Q_SIGNALS:
        void groupChanged(Id<Group>);

    public Q_SLOTS:
        void setGroup(const Id<Group>& id);

    private:
        Id<Group> m_id;
};

}
