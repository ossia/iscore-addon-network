#pragma once
#include <iscore/model/IdentifiedObject.hpp>
#include <vector>

#include <iscore/model/Identifier.hpp>
class QObject;

namespace Network
{
class Group;
class Client;
class GroupManager : public IdentifiedObject<GroupManager>
{
        Q_OBJECT
    public:
        explicit GroupManager(QObject* parent);

        template<typename Deserializer>
        GroupManager(Deserializer&& vis, QObject* parent) :
            IdentifiedObject{vis, parent}
        {
            vis.writeTo(*this);
        }

        void addGroup(Group* group);
        void removeGroup(Id<Group> group);

        const std::vector<Group*>& groups() const;
        Group* findGroup(const QString& str) const;
        Group* group(const Id<Group>& id) const;
        Id<Group> defaultGroup() const;


        //! Number of clients in all the groups
        std::size_t clientsCount(const std::vector<Id<Group>>& grps);
        std::vector<Id<Client>> clients(const std::vector<Id<Group>>& grps);

    signals:
        void groupAdded(Id<Group>);
        void groupRemoved(Id<Group>);

    private:
        std::vector<Group*> m_groups;
};
}
