#pragma once
#include <score/model/Identifier.hpp>
#include <score/tools/std/Optional.hpp>

#include <QTableWidget>

namespace Network
{
class Group;

class GroupHeaderItem : public QTableWidgetItem
{
public:
  explicit GroupHeaderItem(const Group& group);

  const Id<Group> group;
};
}
