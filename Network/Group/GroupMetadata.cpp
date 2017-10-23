#include <score/serialization/VisitorCommon.hpp>

#include "GroupMetadata.hpp"

struct VisitorVariant;

namespace Network
{

GroupMetadata::GroupMetadata(
    Id<Component> self,
    Id<Group> id,
    QObject* parent):
  score::SerializableComponent{self, "GroupMetadata", parent},
  m_id{id}
{

}

GroupMetadata::~GroupMetadata()
{

}

void GroupMetadata::setGroup(const Id<Group>& id)
{
  if(id != m_id)
  {
    m_id = id;
    emit groupChanged(id);
  }
}

}
