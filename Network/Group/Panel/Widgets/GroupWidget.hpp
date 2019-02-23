#pragma once
#include <score/model/Identifier.hpp>

#include <QWidget>

namespace Network
{
class Group;

class GroupWidget final : public QWidget
{
public:
  GroupWidget(Group* group, QWidget* parent);

  Id<Group> id() const;

private:
  Group* m_group;
};
}
