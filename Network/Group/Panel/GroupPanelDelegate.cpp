#include "GroupPanelDelegate.hpp"

#include <Network/Group/Commands/CreateGroup.hpp>
#include <Network/Group/Panel/Widgets/GroupListWidget.hpp>
#include <Network/Group/Panel/Widgets/GroupTableWidget.hpp>
#include <Network/Group/GroupManager.hpp>
#include <Network/Document/DocumentPlugin.hpp>

#include <Network/Session/Session.hpp>

#include <iscore/document/DocumentInterface.hpp>

#include <QLabel>
#include <QPushButton>
#include <QInputDialog>
#include <QVBoxLayout>

namespace Network
{
PanelDelegate::PanelDelegate(const iscore::GUIApplicationContext& ctx):
  iscore::PanelDelegate{ctx},
  m_widget{new QWidget}
{
  new QVBoxLayout{m_widget};
}

QWidget* PanelDelegate::widget()
{
  return m_widget;
}

const iscore::PanelStatus&PanelDelegate::defaultPanelStatus() const
{
  static const iscore::PanelStatus status{
    false,
    Qt::RightDockWidgetArea,
        1,
        QObject::tr("Groups"),
        QObject::tr("Ctrl+G")};

  return status;
}

void PanelDelegate::on_modelChanged(
    iscore::MaybeDocument oldm,
    iscore::MaybeDocument newm)
{
  disconnect(m_con);
  if(!newm)
  {
    setEmptyView();
    return;
  }

  NetworkDocumentPlugin* netplug = newm->findPlugin<NetworkDocumentPlugin>();

  if(netplug)
  {
    if(!netplug->policy())
      return;

    m_con = connect(netplug, &NetworkDocumentPlugin::sessionChanged,
                    this, [=] () {
      setView(netplug->groupManager(), netplug->policy()->session());
    });

    setView(netplug->groupManager(), netplug->policy()->session());
  }


}

void PanelDelegate::setView(
    const GroupManager& mgr,
    const Session* session)
{
  // Make the containing widget
  delete m_subWidget;
  m_subWidget = new QWidget;

  auto lay = new QVBoxLayout;
  m_subWidget->setLayout(lay);

  m_widget->layout()->addWidget(m_subWidget);

  // The sub-widgets (group data presentation)
  m_subWidget->layout()->addWidget(new QLabel{session->metaObject()->className()});
  m_subWidget->layout()->addWidget(new GroupListWidget{mgr, m_subWidget});

  // Add group button
  auto button = new QPushButton{QObject::tr("Add group")};
  ObjectPath mgrpath{iscore::IDocument::unsafe_path(mgr)};
  connect(button, &QPushButton::pressed, this, [=] ( )
  {
    if(auto doc = this->document())
    {
      bool ok;
      QString text = QInputDialog::getText(m_widget, tr("New group"),
                                           tr("Group name:"), QLineEdit::Normal, "", &ok);
      if (ok && !text.isEmpty())
      {
        auto cmd = new Command::CreateGroup{ObjectPath{mgrpath}, text};

        CommandDispatcher<> dispatcher{doc->commandStack};
        dispatcher.submitCommand(cmd);
      }
    }
  });
  m_subWidget->layout()->addWidget(button);

  // Group table
  m_subWidget->layout()->addWidget(new GroupTableWidget{mgr, session, m_widget});
}

void PanelDelegate::setEmptyView()
{
  delete m_subWidget;
  m_subWidget = nullptr;
}

void PanelDelegate::networkPluginReady()
{
  on_modelChanged(document(), document());
}


}
