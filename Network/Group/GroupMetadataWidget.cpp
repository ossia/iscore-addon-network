
#include "GroupMetadata.hpp"

/*
#include "GroupMetadataWidget.hpp"

#include "Commands/ChangeGroup.hpp"
#include "Group.hpp"
#include "GroupManager.hpp"
#include "GroupMetadata.hpp"

#include <score/command/Dispatchers/CommandDispatcher.hpp>
#include <score/document/DocumentContext.hpp>
#include <score/document/DocumentInterface.hpp>
#include <score/model/Identifier.hpp>
#include <score/tools/std/Optional.hpp>
#include <score/tools/Bind.hpp>
#include <score/widgets/SignalUtils.hpp>

#include <QBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QLayout>
#include <QString>
#include <QVariant>

#include <vector>

Q_DECLARE_METATYPE(Id<Network::Group>)

namespace Network
{
GroupMetadataWidget::GroupMetadataWidget(
        const GroupMetadata& groupmetadata,
        const GroupManager* mgr,
        QWidget* widg):
    QWidget{widg},
    m_object{groupmetadata},
    m_groups{mgr}
{
    this->setLayout(new QHBoxLayout);
    this->layout()->addWidget(new QLabel{tr("Groups: ")});

    con(groupmetadata, &GroupMetadata::groupChanged,
            this, [=] (const Id<Group>& grp)
    {
        updateLabel(grp);
    });

    connect(m_groups, &GroupManager::groupAdded,
            this, &GroupMetadataWidget::on_groupAdded);
    connect(m_groups, &GroupManager::groupRemoved,
            this, &GroupMetadataWidget::on_groupRemoved);

    updateLabel(groupmetadata.group());
}

void GroupMetadataWidget::on_groupAdded(const Id<Group>& id)
{
    m_combo->addItem(m_groups->group(id)->name(), QVariant::fromValue(id));
}

void GroupMetadataWidget::on_groupRemoved(const Id<Group>& id)
{
    int index = m_combo->findData(QVariant::fromValue(id));
    m_combo->removeItem(index);
}

void GroupMetadataWidget::on_indexChanged(int)
{
    auto data = m_combo->currentData().value<Id<Group>>();
    if(m_object.group() != data)
    {
      auto& ctx = score::IDocument::documentContext(*m_groups);
      CommandDispatcher<> dispatcher{ctx.commandStack};
      dispatcher.submit(
            new Command::ChangeGroup{
              score::IDocument::unsafe_path(m_object.element()),
              data});
    }
}

void GroupMetadataWidget::updateLabel(const Id<Group>& currentGroup)
{
    delete m_combo;
    m_combo = new QComboBox;

    for(unsigned int i = 0; i < m_groups->groups().size(); i++)
    {
        m_combo->addItem(m_groups->groups()[i]->name(),
                         QVariant::fromValue(m_groups->groups()[i]->id()));

        if(m_groups->groups()[i]->id() == currentGroup)
        {
            m_combo->setCurrentIndex(i);
        }
    }

    connect(m_combo, SignalUtils::QComboBox_currentIndexChanged_int(),
            this, &GroupMetadataWidget::on_indexChanged);

    this->layout()->addWidget(m_combo);
}
}

*/
