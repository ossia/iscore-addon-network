#pragma once

#include <score/command/Command.hpp>

#include <QApplication>
namespace Network
{
class ClientNameChangedCommand : public score::Command
{
  // QUndoCommand interface
public:
  /*
  ClientNameChangedCommand(QString oldval, QString newval) :
      Command {QString(""),
              QString(""),
              QString("")
  },
  m_oldval {oldval},
  m_newval {newval}
  {

  }
*/
  /*
  bool mergeWith(const Command* other) override
  {
      auto cmd = static_cast<const ClientNameChangedCommand*>(other);
      m_newval = cmd->m_newval;
      return true;
  }
  */

  void undo(const score::DocumentContext& ctx) const override
  {
    auto target = qApp->findChild<NetworkSettingsModel*>("NetworkSettingsModel");
    target->setClientName(m_oldval);
  }

  void redo(const score::DocumentContext& ctx) const override
  {
    auto target = qApp->findChild<NetworkSettingsModel*>("NetworkSettingsModel");
    target->setClientName(m_newval);
  }

private:
  QString m_oldval;
  QString m_newval;
};
}
