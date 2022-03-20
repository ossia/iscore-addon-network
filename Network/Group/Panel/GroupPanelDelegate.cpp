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

class ClientListWidget : public QWidget
{
  Session& m_session;
  QVector<QWidget*> m_clients;

public:
  ClientListWidget(const Session& s, QWidget* p)
      : QWidget{p}, m_session{const_cast<Session&>(s)}
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

    const auto& str = Network::Constants::instance();
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
    {
      auto g = new QButtonGroup{this};
      auto async = setup("Async", l1, g, [=] {
        disp.submit(new SetSyncMode{this->m_plug, m_ctx.selectionStack.currentSelection(), SyncMode::NonCompensatedAsync});
      });
      auto sync = setup("Sync", l1, g, [=] {
        disp.submit(new SetSyncMode{this->m_plug, m_ctx.selectionStack.currentSelection(), SyncMode::NonCompensatedSync});
      });
      if(init.syncmode==SyncMode::CompensatedAsync || init.syncmode==SyncMode::NonCompensatedAsync)
        async->toggle();
      else
        sync->toggle();
    }

    auto l2 = new QHBoxLayout{};
    {
      auto g = new QButtonGroup{this};
      auto shared = setup("Shared", l2, g, [=] {
        disp.submit(new SetShareMode{this->m_plug, m_ctx.selectionStack.currentSelection(), ShareMode::Shared});
      });
      auto mixed = setup("Mixed", l2, g, [=] {
        disp.submit(new SetShareMode{this->m_plug, m_ctx.selectionStack.currentSelection(), ShareMode::Mixed});
      });
      auto free = setup("Free", l2, g, [=] {
        disp.submit(new SetShareMode{this->m_plug, m_ctx.selectionStack.currentSelection(), ShareMode::Free});
      });
      if(init.sharemode==ShareMode::Shared)
        shared->toggle();
      else if(init.sharemode==ShareMode::Mixed)
        mixed->toggle();
      else if(init.sharemode==ShareMode::Free)
        free->toggle();
    }

    auto l3 = new QHBoxLayout{};
    {
      auto g = new QButtonGroup{this};
      auto ordered = setup("Ordered", l3, g, [=] {
        disp.submit(new SetOrderedMode{this->m_plug, m_ctx.selectionStack.currentSelection(), true});
      });
      auto unordered = setup("Unordered", l3, g, [=] {
        disp.submit(new SetOrderedMode{this->m_plug, m_ctx.selectionStack.currentSelection(), false});
      });
      if(init.ordered)
        ordered->toggle();
      else
        unordered->toggle();
    }

    auto l4 = new QVBoxLayout{};
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

    lay->addLayout(l1);
    lay->addLayout(l2);
    lay->addLayout(l3);
    lay->addLayout(l4);
    lay->addSpacing(1);
  }
};

void PanelDelegate::setView(
    const score::DocumentContext& ctx,
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
  m_subWidget->layout()->addWidget(
      new QLabel{session->metaObject()->className()});
  m_subWidget->layout()->addWidget(new GroupListWidget{mgr, m_subWidget});

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
  m_subWidget->layout()->addWidget(button);

  // Group table
  m_subWidget->layout()->addWidget(new score::HSeparator{m_subWidget});
  auto transport_widg = new QWidget;
  auto transport_lay = new QHBoxLayout{transport_widg};
  auto play = new QPushButton{tr("Play")};
  auto stop = new QPushButton{tr("Stop")};

  connect(play, &QPushButton::clicked, this, [=] {
    auto act = context().actions.action<Actions::NetworkPlay>().action();
    act->trigger();
  });
  connect(stop, &QPushButton::clicked, this, [=] {
    auto act = context().actions.action<Actions::NetworkStop>().action();
    act->trigger();
  });

  transport_lay->addWidget(play);
  transport_lay->addWidget(stop);

  m_subWidget->layout()->addWidget(transport_widg);
  m_subWidget->layout()->addWidget(new ClientListWidget{*session, m_widget});
  m_subWidget->layout()->addWidget(new score::HSeparator{m_subWidget});
  m_subWidget->layout()->addWidget(
      new NetworkMetadataWidget{ctx, mgr, m_widget});
  m_subWidget->layout()->addWidget(
      new GroupTableWidget{mgr, session, m_widget});
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
