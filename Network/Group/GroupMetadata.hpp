#pragma once
/*

#include <score/model/ComponentSerialization.hpp>
#include <score/model/Identifier.hpp>
#include <score/tools/std/Optional.hpp>
#include <Network/Group/Group.hpp>
#include <QObject>
#include <QString>

struct VisitorVariant;

namespace Network
{
class Group;
//! Goes into the intervals, events, etc.
class GroupMetadata : public score::Component
{
  W_OBJECT(GroupMetadata)

public:
  GroupMetadata(Id<Group> id, QObject* parent);

  ~GroupMetadata();

  template <typename DeserializerVisitor>
  GroupMetadata(DeserializerVisitor&& vis, QObject* parent)
      : score::Component{vis, parent}
  {
    vis.writeTo(*this);
  }

  const auto& group() const { return m_id; }

  void groupChanged(Id<Group> g) W_SIGNAL(groupChanged, g);

  void setGroup(const Id<Group>& id);
  W_SLOT(setGroup);

private:
  Id<Group> m_id;
};
}

*/
