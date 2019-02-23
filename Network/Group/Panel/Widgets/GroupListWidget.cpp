#include "GroupListWidget.hpp"

#include "GroupWidget.hpp"

#include <score/model/Identifier.hpp>
#include <score/tools/std/Optional.hpp>

#include <QBoxLayout>
#include <QLayout>

#include <Network/Group/GroupManager.hpp>

#include <algorithm>
#include <iterator>

namespace Network
{
GroupListWidget::GroupListWidget(const GroupManager& mgr, QWidget* parent)
    : QWidget{parent}, m_mgr{mgr}
{
  this->setLayout(new QVBoxLayout);
  for (auto& group : m_mgr.groups())
  {
    auto widg = new GroupWidget{group, this};
    this->layout()->addWidget(widg);
    m_widgets.append(widg);
  }

  con(m_mgr, &GroupManager::groupAdded, this, &GroupListWidget::addGroup);
  con(m_mgr, &GroupManager::groupRemoved, this, &GroupListWidget::removeGroup);
}

void GroupListWidget::addGroup(const Id<Group>& id)
{
  auto widg = new GroupWidget{m_mgr.group(id), this};
  this->layout()->addWidget(widg);
  m_widgets.append(widg);
}

void GroupListWidget::removeGroup(const Id<Group>& id)
{
  using namespace std;
  auto it = find_if(begin(m_widgets), end(m_widgets), [&](GroupWidget* widg) {
    return widg->id() == id;
  });

  m_widgets.removeOne(*it);
  delete *it;
}
}
