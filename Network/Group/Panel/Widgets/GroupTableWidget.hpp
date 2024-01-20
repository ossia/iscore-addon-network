#pragma once
#include <score/command/Dispatchers/CommandDispatcher.hpp>
#include <score/model/path/ObjectPath.hpp>

#include <QWidget>

class QTableWidget;
#include <score/model/Identifier.hpp>

namespace Network
{
class Session;
class Client;
class GroupManager;
class GroupTableCheckbox;
class GroupTableWidget : public QWidget
{
public:
  GroupTableWidget(const GroupManager& mgr, const Session* session, QWidget* parent);

  void setup();

private:
  GroupTableCheckbox* findCheckbox(int i, Id<Client> theClient) const;

  void on_checkboxChanged(int i, int j, int state);

  QTableWidget* m_table{};
  const GroupManager& m_mgr;
  const Session* m_session;

  CommandDispatcher<> m_dispatcher;
};
}
