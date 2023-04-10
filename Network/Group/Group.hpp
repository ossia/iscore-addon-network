#pragma once
#include <score/model/IdentifiedObject.hpp>
#include <score/model/Identifier.hpp>
#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>
#include <score/tools/std/Optional.hpp>

#include <QString>

#include <Network/Client/Client.hpp>

#include <vector>

class DataStream;
class JSONObject;
class QObject;

namespace Network
{

// Groups : registered in the session
// Permissions ? for now we will just have, for each interval in a score,
// a chosen group.
// There is a "default" group that runs the interval everywhere.
// The groups are part of the document plugin.
// Each client can be in a group (it will execute all the intervals that are
// part of this group).

class Group : public IdentifiedObject<Group>
{
  W_OBJECT(Group)
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

  SCORE_SERIALIZE_FRIENDS
public:
  Group(QString name, Id<Group> id, QObject* parent);

  template <typename Deserializer>
  Group(Deserializer&& vis, QObject* parent)
      : IdentifiedObject{vis, parent}
  {
    vis.writeTo(*this);
  }

  QString name() const;
  void setName(QString arg);

  void addClient(Id<Client> clt);
  void removeClient(Id<Client> clt);
  bool hasClient(const Id<Client>& clt) const;

  const std::vector<Id<Client>>& clients() const { return m_executingClients; }

  void nameChanged(QString arg) W_SIGNAL(nameChanged, arg);

  void clientAdded(Id<Client> arg) W_SIGNAL(clientAdded, arg);
  void clientRemoved(Id<Client> arg) W_SIGNAL(clientRemoved, arg);

private:
  QString m_name;

  std::vector<Id<Client>> m_executingClients;
};
}
W_REGISTER_ARGTYPE(Id<Network::Group>)
