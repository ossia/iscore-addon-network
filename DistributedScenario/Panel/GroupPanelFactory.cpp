#include "GroupPanelFactory.hpp"

#include <DistributedScenario/Commands/CreateGroup.hpp>
#include <DistributedScenario/Panel/Widgets/GroupListWidget.hpp>
#include <DistributedScenario/Panel/Widgets/GroupTableWidget.hpp>
#include <DistributedScenario/GroupManager.hpp>
#include <DocumentPlugins/NetworkDocumentPlugin.hpp>

#include <Repartition/session/Session.hpp>

#include <iscore/document/DocumentInterface.hpp>

#include <QLabel>
#include <QPushButton>
#include <QInputDialog>
#include <QVBoxLayout>

namespace Network
{
PanelDelegate::PanelDelegate(const iscore::ApplicationContext& ctx):
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

std::unique_ptr<iscore::PanelDelegate> PanelDelegateFactory::make(
        const iscore::ApplicationContext& ctx)
{
    return std::make_unique<PanelDelegate>(ctx);
}


void PanelDelegate::on_modelChanged(
        iscore::PanelDelegate::maybe_document_t oldm,
        iscore::PanelDelegate::maybe_document_t newm)
{
    disconnect(m_con);
    if(!newm)
    {
        setEmptyView();
        return;
    }

    auto netplug = newm->findPlugin<NetworkDocumentPlugin>();

    if(netplug)
    {
        if(!netplug->policy())
            return;

        m_con = connect(netplug, &NetworkDocumentPlugin::sessionChanged,
                this, [=] () {
            auto currentManager = netplug->groupManager();
            auto currentSession = netplug->policy()->session();

            if(currentManager)
            {
                setView(currentManager, currentSession);
            }
            else
            {
                setEmptyView();
            }
        });

        auto currentManager = netplug->groupManager();
        auto currentSession = netplug->policy()->session();
        if(currentManager)
        {
            setView(currentManager, currentSession);
        }
        else
        {
            setEmptyView();
        }
    }


}

void PanelDelegate::setView(
        const GroupManager* mgr,
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
        bool ok;
        QString text = QInputDialog::getText(m_widget, tr("New group"),
                                             tr("Group name:"), QLineEdit::Normal, "", &ok);
        if (ok && !text.isEmpty())
        {
            auto cmd = new Command::CreateGroup{ObjectPath{mgrpath}, text};

            CommandDispatcher<> dispatcher{
                iscore::IDocument::documentContext(*mgr).commandStack
            };
            dispatcher.submitCommand(cmd);
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


}
