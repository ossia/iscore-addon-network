#include "GroupPanelDelegate.hpp"

#include <Network/Group/Commands/CreateGroup.hpp>
#include <Network/Group/Panel/Widgets/GroupListWidget.hpp>
#include <Network/Group/Panel/Widgets/GroupTableWidget.hpp>
#include <Network/Group/GroupManager.hpp>
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Session/Session.hpp>
#include <Network/Group/NetworkActions.hpp>
#include <iscore/actions/ActionManager.hpp>
#include <iscore/document/DocumentInterface.hpp>
#include <iscore/widgets/ClearLayout.hpp>
#include <iscore/widgets/MarginLess.hpp>
#include <iscore/widgets/Separator.hpp>

#include <QLabel>
#include <QPushButton>
#include <QInputDialog>
#include <QVBoxLayout>

#include <QGridLayout>
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
    m_con = connect(netplug, &NetworkDocumentPlugin::sessionChanged,
                    this, [=] () {
      setView(netplug->groupManager(), netplug->policy().session());
    });

    setView(netplug->groupManager(), netplug->policy().session());
  }


}
class ClientListWidget: public QWidget
{
  Session& m_session;
  QVector<QWidget*> m_clients;
public:
  ClientListWidget(const Session& s, QWidget* p)
    : QWidget{p}
    , m_session{const_cast<Session&>(s)}
  {
    recompute();
    connect(&s, &Session::clientAdded, this, [=] { recompute(); });
    connect(&s, &Session::clientRemoved, this, [=] { recompute(); });
  }

  void recompute()
  {
    iscore::clearLayout(this->layout());
    m_clients.clear();
    delete this->layout();

    auto lay = new iscore::MarginLess<QGridLayout>;
    this->setLayout(lay);

    int i = 0;
    for(auto& c : m_session.remoteClients())
    {
      auto name = new QLabel(c->name());
      auto play = new QPushButton(tr("Play"));
      auto stop = new QPushButton(tr("Stop"));

      connect(play, &QPushButton::clicked, this, [=] {
        auto& mapi = MessagesAPI::instance();
        m_session.sendMessage(c->id(), m_session.makeMessage(mapi.play));
      });
      connect(stop, &QPushButton::clicked, this, [=] {
        auto& mapi = MessagesAPI::instance();
        m_session.sendMessage(c->id(), m_session.makeMessage(mapi.stop));
      });
      lay->addWidget(name, i, 0);
      lay->addWidget(play, i, 1);
      lay->addWidget(stop, i, 2);

      i++;
    }
  }

};

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
  m_subWidget->layout()->addWidget(new iscore::HSeparator{m_subWidget});
  auto transport_widg = new QWidget;
  auto transport_lay = new QHBoxLayout{transport_widg};
  auto play = new QPushButton{tr("Play")};
  auto stop = new QPushButton{tr("Stop")};

  connect(play, &QPushButton::clicked,
          this, [=] {
    auto act = context().actions.action<Actions::NetworkPlay>().action();
    act->trigger();
  });
  connect(play, &QPushButton::clicked,
          this, [=] {
    auto act = context().actions.action<Actions::NetworkStop>().action();
    act->trigger();
  });

  transport_lay->addWidget(play);
  transport_lay->addWidget(stop);

  m_subWidget->layout()->addWidget(transport_widg);
  m_subWidget->layout()->addWidget(new ClientListWidget{*session, m_widget});

  // Clients
  m_subWidget->layout()->addWidget(new iscore::HSeparator{m_subWidget});
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
