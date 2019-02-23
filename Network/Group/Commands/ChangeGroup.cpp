/*
#include "ChangeGroup.hpp"

#include <Scenario/Document/Event/EventModel.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>

#include <score/model/path/ObjectPath.hpp>
#include <score/plugins/documentdelegate/plugin/ElementPluginModel.hpp>
#include <score/plugins/documentdelegate/plugin/ElementPluginModelList.hpp>
#include <score/serialization/DataStreamVisitor.hpp>

#include <boost/concept/usage.hpp>
#include <boost/range/algorithm/find_if.hpp>

#include <QObject>
#include <QString>

#include <vector>

#include <Network/Group/GroupMetadata.hpp"


namespace Network
{
class Group;

namespace Command
{
static GroupMetadata* getGroupMetadata(QObject* obj)
{
    if(auto cstr = dynamic_cast<Scenario::IntervalModel*>(obj))
    {
        auto& plugs = cstr->pluginModelList.list();
        auto plug_it = find_if(plugs, [] (score::ElementPluginModel* elt)
        { return elt->metaObject()->className() == QString{"GroupMetadata"};
}); SCORE_ASSERT(plug_it != plugs.end());

        return static_cast<GroupMetadata*>(*plug_it);
    }
    else if(auto ev = dynamic_cast<Scenario::EventModel*>(obj))
    {
        auto& plugs = ev->pluginModelList.list();
        auto plug_it = find_if(plugs, [] (score::ElementPluginModel* elt)
        { return elt->metaObject()->className() == QString{"GroupMetadata"};
}); SCORE_ASSERT(plug_it != plugs.end());

        return static_cast<GroupMetadata*>(*plug_it);
    }
    score_ABORT;
    return nullptr;
}

ChangeGroup::ChangeGroup(ObjectPath &&path, Id<Group> newGroup):
    m_path{path},
    m_newGroup{newGroup}
{
    m_oldGroup = getGroupMetadata(&m_path.find<QObject>())->group();
}

void ChangeGroup::undo(const score::DocumentContext& ctx) const
{
    getGroupMetadata(&m_path.find<QObject>())->setGroup(m_oldGroup);
}

void ChangeGroup::redo(const score::DocumentContext& ctx) const
{
    getGroupMetadata(&m_path.find<QObject>())->setGroup(m_newGroup);
}

void ChangeGroup::serializeImpl(DataStreamInput &s) const
{
    s << m_path << m_newGroup << m_oldGroup;
}

void ChangeGroup::deserializeImpl(DataStreamOutput &s)
{
    s >> m_path >> m_newGroup >> m_oldGroup;
}
}
}

*/
