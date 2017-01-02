#pragma once

#include <iscore/model/ComponentSerialization.hpp>
#include <iscore/tools/std/Optional.hpp>
#include <QObject>

#include <QString>


#include <iscore/model/Identifier.hpp>

struct VisitorVariant;

namespace Network
{
class Group;
//! Goes into the constraints, events, etc.
class GroupMetadata :
    public iscore::SerializableComponent
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
            iscore::SerializableComponent{parent}
        {
            vis.writeTo(*this);
        }

        const auto& group() const
        { return m_id; }

    signals:
        void groupChanged(Id<Group>);

    public slots:
        void setGroup(const Id<Group>& id);

    private:
        Id<Group> m_id;
};

}
