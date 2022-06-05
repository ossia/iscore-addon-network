#include "GroupPanelDelegate.hpp"
#include <score/selection/SelectionStack.hpp>

#include <score/actions/ActionManager.hpp>
#include <score/document/DocumentInterface.hpp>
#include <score/widgets/ClearLayout.hpp>
#include <score/widgets/MarginLess.hpp>
#include <score/widgets/Separator.hpp>
#include <core/document/Document.hpp>


#include <QButtonGroup>
#include <QGridLayout>
#include <QInputDialog>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QTabWidget>
#include <QCheckBox>

#include <score/application/GUIApplicationContext.hpp>
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Document/Execution/Context.hpp>
#include <Network/Group/Commands/AddCustomMetadata.hpp>
#include <Network/Group/Commands/CreateGroup.hpp>
#include <Network/Group/GroupManager.hpp>
#include <Network/Group/NetworkActions.hpp>
#include <Network/Group/Panel/Widgets/GroupListWidget.hpp>
#include <Network/Group/Panel/Widgets/GroupTableWidget.hpp>
#include <Network/Session/Session.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/Event/EventModel.hpp>
#include <Scenario/Document/TimeSync/TimeSyncModel.hpp>
#include <Process/Process.hpp>
namespace Network
{
PanelDelegate::PanelDelegate(const score::GUIApplicationContext& ctx)
    : score::PanelDelegate{ctx}, m_widget{new QWidget}
{
  new QVBoxLayout{m_widget};
}

QWidget* PanelDelegate::widget()
{
  return m_widget;
}

const score::PanelStatus& PanelDelegate::defaultPanelStatus() const
{
  static const score::PanelStatus status{false, false,
                                         Qt::LeftDockWidgetArea,
                                         1,
                                         QObject::tr("Groups"),
                                         "device_explorer",
                                         QObject::tr("Ctrl+G")};

  return status;
}

void PanelDelegate::on_modelChanged(
    score::MaybeDocument oldm,
    score::MaybeDocument newm)
{
  disconnect(m_con);
  if (!newm)
  {
    setEmptyView();
    return;
  }

  NetworkDocumentPlugin* netplug = newm->findPlugin<NetworkDocumentPlugin>();

  if (netplug)
  {
    m_con = connect(
        netplug, &NetworkDocumentPlugin::sessionChanged, this, [=]() {
          setView(*newm, netplug->groupManager(), netplug->policy().session());
        });

    setView(*newm, netplug->groupManager(), netplug->policy().session());
  }
}

class ClientListWidget : public QGroupBox
{
  Session& m_session;
  QVector<QWidget*> m_clients;

public:
  ClientListWidget(const Session& s, QWidget* p)
      : QGroupBox{tr("Clients"), p}, m_session{const_cast<Session&>(s)}
  {
    recompute();
    connect(&s, &Session::clientAdded, this, [=] { recompute(); });
    connect(&s, &Session::clientRemoved, this, [=] { recompute(); });
  }

  void recompute()
  {
    QWidget{}.setLayout(this->layout());
    m_clients.clear();

    auto lay = new score::MarginLess<QGridLayout>;
    this->setLayout(lay);

    int i = 0;
    for (auto& c : m_session.remoteClients())
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

class NetworkMetadataWidget : public QWidget
{
  const score::DocumentContext& m_ctx;
  const GroupManager& m_mgr;
  NetworkDocumentPlugin& m_plug;
  CommandDispatcher<> disp{m_ctx.commandStack};

public:
  NetworkMetadataWidget(
      const score::DocumentContext& ctx,
      const GroupManager& mgr,
      QWidget* parent)
      : QWidget{parent}
      , m_ctx{ctx}
      , m_mgr{mgr}
      , m_plug{m_ctx.plugin<NetworkDocumentPlugin>()}
  {
    connect(&mgr, &GroupManager::groupAdded, this, [=] { recompute(); });
    connect(&mgr, &GroupManager::groupRemoved, this, [=] { recompute(); });
    connect(&m_ctx.selectionStack, &score::SelectionStack::currentSelectionChanged,
            this, [=] { recompute(); });
    recompute();
  }

  void recompute()
  {
    const Selection& sel = m_ctx.selectionStack.currentSelection();
    if(sel.empty())
      return;

    QObject* obj = *sel.begin();
    ObjectMetadata init{};
    enum { Other, Interval, Event, Sync, Process } selectedType{Other};
    if(auto itv = qobject_cast<Scenario::IntervalModel*>(obj)) {
      if(auto m = m_plug.get_metadata(*itv))
        init = *m;
      selectedType = Interval;
    }
    else if(auto e = qobject_cast<Scenario::EventModel*>(obj)) {
      if(auto m = m_plug.get_metadata(*e))
        init = *m;
      selectedType = Event;
    }
    else if(auto ts = qobject_cast<Scenario::TimeSyncModel*>(obj)) {
      if(auto m = m_plug.get_metadata(*ts))
        init = *m;
      selectedType = Sync;
    }
    else if(auto p = qobject_cast<Process::ProcessModel*>(obj)) {
      if(auto m = m_plug.get_metadata(*p))
        init = *m;
      selectedType = Process;
    }
    if(selectedType == Other)
      return;

    if(init.group.isEmpty())
      init.group = "all";

    auto setup = [=](const QString& txt, auto l, auto g, auto func) {
      auto btn = new QRadioButton{txt};
      g->addButton(btn);
      l->addWidget(btn);
      connect(btn, &QRadioButton::clicked, this, func);
      return btn;
    };
    if (auto l = this->layout())
    {
      QWidget{}.setLayout(this->layout());
    }

    using namespace Command;
    auto lay = new QVBoxLayout{this};
    auto l1 = new QHBoxLayout{};
    auto l1bis = new QHBoxLayout{};

    lay->addWidget(new QLabel{"Expression Synchronization"});
    lay->addLayout(l1);
    lay->addLayout(l1bis);
    {
      auto g = new QButtonGroup{this};
      auto async_uc = setup("Async", l1, g, [=] {
        disp.submit(new SetSyncMode{this->m_plug, m_ctx.selectionStack.currentSelection(), SyncMode::NonCompensatedAsync});
      });
      // auto async_c = setup("Async (Compensated)", l1, g, [=] {
      //   disp.submit(new SetSyncMode{this->m_plug, m_ctx.selectionStack.currentSelection(), SyncMode::CompensatedAsync});
      // });
      auto sync_uc = setup("Sync", l1bis, g, [=] {
        disp.submit(new SetSyncMode{this->m_plug, m_ctx.selectionStack.currentSelection(), SyncMode::NonCompensatedSync});
      });
      // auto sync_c = setup("Sync (Compensated)", l1bis, g, [=] {
      //   disp.submit(new SetSyncMode{this->m_plug, m_ctx.selectionStack.currentSelection(), SyncMode::CompensatedSync});
      // });
      switch(init.syncmode)
      {
        case SyncMode::NonCompensatedAsync:
          async_uc->toggle(); break;
        // case SyncMode::CompensatedAsync:
        //   async_c->toggle(); break;
        case SyncMode::NonCompensatedSync:
          sync_uc->toggle(); break;
        // case SyncMode::CompensatedSync:
        //   sync_c->toggle(); break;
      }
    }

    auto l2 = new QHBoxLayout{};
    lay->addWidget(new QLabel{"Scenario sharing"});
    lay->addLayout(l2);
    {
      auto g = new QButtonGroup{this};
      auto shared = setup("Shared", l2, g, [=] {
        disp.submit(new SetShareMode{this->m_plug, m_ctx.selectionStack.currentSelection(), ShareMode::Shared});
      });
      // auto mixed = setup("Mixed", l2, g, [=] {
      //   disp.submit(new SetShareMode{this->m_plug, m_ctx.selectionStack.currentSelection(), ShareMode::Mixed});
      // });
      auto free = setup("Free", l2, g, [=] {
        disp.submit(new SetShareMode{this->m_plug, m_ctx.selectionStack.currentSelection(), ShareMode::Free});
      });
      if(init.sharemode==ShareMode::Shared)
        shared->toggle();
      //else if(init.sharemode==ShareMode::Mixed)
      //  mixed->toggle();
      else if(init.sharemode==ShareMode::Free)
        free->toggle();
    }


    auto l4 = new QVBoxLayout{};
    lay->addWidget(new QLabel{"Group"});
    lay->addLayout(l4);
    lay->addSpacing(10);
    {
      auto g = new QButtonGroup{this};
      auto parent_g = setup("Parent", l4, g, [=] {
        disp.submit(new SetGroup{this->m_plug, m_ctx.selectionStack.currentSelection(), "parent"});
      });
      if(init.group == "parent")
        parent_g->toggle();

      for (Group* group : m_mgr.groups())
      {
        auto child_g = setup(group->name(), l4, g, [=, n = group->name()] {
          disp.submit(new SetGroup{this->m_plug, m_ctx.selectionStack.currentSelection(), n});
        });

        if(init.group == group->name())
          child_g->toggle();
      }
    }


    auto helplabel = new QLabel;
    helplabel->setTextFormat(Qt::RichText);
    helplabel->setText(tr(
R"_(<b>Network configuration</b><br/><br/>

<b>Async / sync</b>: relevant for triggers & conditions.<br/>
- <b>Sync</b>: expression state is shared.<br/>
- <b>Async</b>: expression can be different across clients.<br/>)_"
// - <b>Compensated</b>: when the trigger is ready, <br/>
//    set its execution date at a date in the future to ensure synchronisation.<br/>
// - <b>Uncompensated</b>: when the trigger is ready, go immediately.<br/>
R"_(<br/><br/>
<b>Shared / mixed / free</b>: relevant for Scenario processes.<br/>
- If shared: the scenario's execution is shared across clients.<br/>
- If free: for every client in the scenario's group, it executes independently.<br/>
<br/><br/>
<b>Group</b>: in which group the object executes.<br/>
<br/>
Two special groups are:<br/>
<br/>
- "parent" (the property is inherited from the parent)<br/>
- "all" (executes on all groups)<br/>
)_"));
    lay->addWidget(helplabel);
    lay->addStretch();
  }
};

void PanelDelegate::setView(
    const score::DocumentContext& ctx,
    const GroupManager& mgr,
    const Session* session)
{
  // Make the containing widget
  delete m_subWidget;
  m_subWidget = new QTabWidget;
  m_widget->layout()->addWidget(m_subWidget);

  auto topology = new QWidget;
  auto topology_layout = new QVBoxLayout{topology};
  m_subWidget->addTab(topology, tr("Topology"));
  // The sub-widgets (group data presentation)
  topology_layout->addWidget(new QLabel{session->metaObject()->className()});
  topology_layout->addWidget(new GroupListWidget{mgr, m_subWidget});

  // Add group button
  auto button = new QPushButton{QObject::tr("Add group")};
  connect(button, &QPushButton::pressed, this, [=, &mgr]() {
    if (auto doc = this->document())
    {
      bool ok;
      QString text = QInputDialog::getText(
          m_widget,
          tr("New group"),
          tr("Group name:"),
          QLineEdit::Normal,
          "",
          &ok);
      if (ok && !text.isEmpty())
      {
        auto cmd = new Command::CreateGroup{mgr, text};

        CommandDispatcher<> dispatcher{doc->commandStack};
        dispatcher.submit(cmd);
      }
    }
  });
  topology_layout->addWidget(button);

  topology_layout->addWidget(new GroupTableWidget{mgr, session, m_widget});

  // Transport

  auto transport_widg = new QWidget;
  auto transport_lay = new QVBoxLayout{transport_widg};
  transport_lay->addWidget(new ClientListWidget{*session, m_widget});
  auto play = new QPushButton{tr("Play (all)")};
  connect(play, &QPushButton::clicked, this, [=] {
    auto act = context().actions.action<Actions::NetworkPlay>().action();
    act->trigger();
  });

  auto stop = new QPushButton{tr("Stop (all)")};
  connect(stop, &QPushButton::clicked, this, [=] {
    auto act = context().actions.action<Actions::NetworkStop>().action();
    act->trigger();
  });

  auto plug = ctx.findPlugin<NetworkDocumentPlugin>();

  auto enableCommands = new QCheckBox{tr("Send edition commands (false: very dangerous!)")};
  enableCommands->setChecked(plug && plug->policy().sendCommands());
  connect(enableCommands, &QCheckBox::toggled, this, [&ctx] (bool state) {
    if(auto plug = ctx.findPlugin<NetworkDocumentPlugin>())
      plug->policy().setSendCommands(state);
  });
  auto enableControls = new QCheckBox{tr("Send control updates (false: dangerous!)")};
  enableControls->setChecked(plug && plug->policy().sendControls());
  connect(enableControls, &QCheckBox::toggled, this, [&ctx] (bool state) {
    if(auto plug = ctx.findPlugin<NetworkDocumentPlugin>())
      plug->policy().setSendControls(state);
  });

  transport_lay->addWidget(play);
  transport_lay->addWidget(stop);
  transport_lay->addWidget(enableCommands);
  transport_lay->addWidget(enableControls);
  transport_lay->addWidget(
        new QLabel{tr(R"_(<br/><br/><b>Warning</b><br/><br/>
Disabling commands means that this instance will not inform <br/>
the server when its local state changes. <br/>
If this is used anywhere in the network, undo<br/>
must not be used at all by anyone.<br/>
<br/>
It is only (barely) safe for small things such as:<br/>
- Changing the address of a port<br/>
- Changing a control<br/>
- Changing a curve's curvature<br/>
<br/>
But not for anything that adds / removes objects.)_")});
  transport_lay->addStretch();

  m_subWidget->addTab(transport_widg, tr("Transport"));
  m_subWidget->addTab(new NetworkMetadataWidget{ctx, mgr, m_widget}, tr("Object"));
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
