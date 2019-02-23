#pragma once
#include <score/model/IdentifiedObject.hpp>
#include <score/model/Identifier.hpp>

#include <vector>
class QObject;

namespace Network
{
class Group;
class Client;
class RemoteClient;
class GroupManager : public IdentifiedObject<GroupManager>
{
  W_OBJECT(GroupManager)
public:
  explicit GroupManager(QObject* parent);

  template <typename Deserializer>
  GroupManager(Deserializer&& vis, QObject* parent)
      : IdentifiedObject{vis, parent}
  {
    vis.writeTo(*this);
  }

  void addGroup(Group* group);
  void removeGroup(Id<Group> group);

  const std::vector<Group*>& groups() const;
  Group* findGroup(const QString& str) const;
  Group* group(const Id<Group>& id) const;
  Id<Group> defaultGroup() const;

  void cleanup(QList<RemoteClient*>);

  //! Number of clients in all the groups
  std::size_t clientsCount(const std::vector<Id<Group>>& grps);
  std::vector<Id<Client>> clients(const std::vector<Id<Group>>& grps);

  void groupAdded(Id<Group> g) W_SIGNAL(groupAdded, g);
  void groupRemoved(Id<Group> g) W_SIGNAL(groupRemoved, g);

private:
  std::vector<Group*> m_groups;
};
}
