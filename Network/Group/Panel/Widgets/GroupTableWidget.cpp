#include "GroupTableWidget.hpp"

#include "GroupHeaderItem.hpp"
#include "GroupTableCheckbox.hpp"
#include "SessionHeaderItem.hpp"

#include <score/command/Dispatchers/CommandDispatcher.hpp>
#include <score/document/DocumentContext.hpp>
#include <score/document/DocumentInterface.hpp>
#include <score/model/Identifier.hpp>
#include <score/model/path/ObjectPath.hpp>
#include <score/tools/std/Optional.hpp>
#include <score/tools/Bind.hpp>

#include <QGridLayout>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QTableWidget>
#include <QVector>
#include <qnamespace.h>

#include <Network/Client/LocalClient.hpp>
#include <Network/Client/RemoteClient.hpp>
#include <Network/Group/Commands/AddClientToGroup.hpp>
#include <Network/Group/Commands/RemoveClientFromGroup.hpp>
#include <Network/Group/Group.hpp>
#include <Network/Group/GroupManager.hpp>
#include <Network/Session/Session.hpp>

#include <vector>

namespace Network
{
GroupTableWidget::GroupTableWidget(
    const GroupManager& mgr,
    const Session* session,
    QWidget* parent)
    : QWidget{parent}
    , m_mgr{mgr}
    , m_session{session}
    , m_managerPath{score::IDocument::unsafe_path(m_mgr)}
    , m_dispatcher{score::IDocument::documentContext(m_mgr).commandStack}
{
  con(m_mgr, &GroupManager::groupAdded, this, &GroupTableWidget::setup);
  con(m_mgr, &GroupManager::groupRemoved, this, &GroupTableWidget::setup);
  connect(m_session, &Session::clientsChanged, this, &GroupTableWidget::setup);

  this->setLayout(new QGridLayout);
  this->layout()->addWidget(new QLabel{"Execution table"});

  setup();
}

void GroupTableWidget::setup()
{
  delete m_table;
  m_table = new QTableWidget;
  this->layout()->addWidget(m_table);

  // Groups
  for (unsigned int i = 0; i < m_mgr.groups().size(); i++)
  {
    m_table->insertColumn(i);
    m_table->setHorizontalHeaderItem(
        i, new GroupHeaderItem{*m_mgr.groups()[i]});
  }

  // Clients
  m_table->insertRow(0); // Local client
  m_table->setVerticalHeaderItem(
      0, new SessionHeaderItem{m_session->localClient()});

  for (int i = 0; i < m_session->remoteClients().size(); i++)
  {
    m_table->insertRow(i + 1);
    m_table->setVerticalHeaderItem(
        i + 1, new SessionHeaderItem{*m_session->remoteClients()[i]});
  }

  // Set the data
  using namespace std;
  for (int row = 0; row < m_session->remoteClients().size() + 1; row++)
  {
    for (unsigned int col = 0; col < m_mgr.groups().size(); col++)
    {
      auto cb = new GroupTableCheckbox;
      m_table->setCellWidget(row, col, cb);
      connect(cb, &GroupTableCheckbox::stateChanged, this, [=](int state) {
        on_checkboxChanged(row, col, state);
      });
    }
  }

  // Handlers
  for (unsigned int i = 0; i < m_mgr.groups().size(); i++)
  {
    Group& group = *m_mgr.groups()[i];
    for (auto& clt : group.clients())
    {
      if (auto cb = findCheckbox(i, clt))
        cb->setState(Qt::Checked);
    }

    con(group, &Group::clientAdded, m_table, [=](Id<Client> addedClient) {
      findCheckbox(i, addedClient)->setState(Qt::Checked);
    });
    con(group, &Group::clientRemoved, m_table, [=](Id<Client> removedClient) {
      findCheckbox(i, removedClient)->setState(Qt::Unchecked);
    });
  }
}

GroupTableCheckbox*
GroupTableWidget::findCheckbox(int i, Id<Client> theClient) const
{
  if (theClient == m_session->localClient().id())
  {
    auto widg = m_table->cellWidget(0, i);
    return static_cast<GroupTableCheckbox*>(widg);
  }

  for (int j = 0; j < m_session->remoteClients().size(); j++)
  {
    if (static_cast<SessionHeaderItem*>(m_table->verticalHeaderItem(j + 1))
            ->client
        == theClient)
    {
      return static_cast<GroupTableCheckbox*>(m_table->cellWidget(j + 1, i));
    }
  }

  qDebug() << "ALERT: checkbox" << i << "not found";
  return nullptr;
}

void GroupTableWidget::on_checkboxChanged(int i, int j, int state)
{
  // Lookup id's from the row / column headers
  auto client = static_cast<SessionHeaderItem*>(m_table->verticalHeaderItem(i))
                    ->client;
  auto group
      = static_cast<GroupHeaderItem*>(m_table->horizontalHeaderItem(j))->group;

  // Find if we have to perform the change.
  auto client_is_in_group = ossia::contains(m_mgr.group(group)->clients(), client);

  if (state)
  {
    if (client_is_in_group)
      return;
    auto cmd = new Command::AddClientToGroup(
        ObjectPath{m_managerPath}, client, group);
    m_dispatcher.submit(cmd);
  }
  else
  {
    if (!client_is_in_group)
      return;
    auto cmd = new Command::RemoveClientFromGroup(
        ObjectPath{m_managerPath}, client, group);
    m_dispatcher.submit(cmd);
  }
}
}
