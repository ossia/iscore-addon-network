#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>
#include <Scenario/Process/ScenarioModel.hpp>
#include <iscore/tools/std/Optional.hpp>
#include <core/document/Document.hpp>
#include <core/document/DocumentModel.hpp>
#include <iscore/serialization/VisitorCommon.hpp>

#include <QString>

#include "DistributedScenario/Group.hpp"
#include "DistributedScenario/GroupManager.hpp"
#include "DistributedScenario/GroupMetadata.hpp"
#include "DistributedScenario/GroupMetadataWidget.hpp"
#include "NetworkDocumentPlugin.hpp"
#include <Process/Process.hpp>
#include "Repartition/session/Session.hpp"
#include <Scenario/Document/Constraint/ConstraintModel.hpp>
#include <Scenario/Document/Event/EventModel.hpp>
#include <iscore/plugins/documentdelegate/plugin/DocumentPlugin.hpp>
#include <iscore/model/EntityMap.hpp>
#include <iscore/model/Identifier.hpp>
#include "session/../client/LocalClient.hpp"

class QWidget;
struct VisitorVariant;

namespace Network
{
NetworkDocumentPlugin::NetworkDocumentPlugin(
    const iscore::DocumentContext& ctx,
    NetworkPolicyInterface *policy,
    Id<iscore::DocumentPlugin> id,
    QObject* parent):
  iscore::SerializableDocumentPlugin{ctx, std::move(id), "NetworkDocumentPlugin", parent},
  m_policy{policy},
  m_groups{new GroupManager{this}}
{
  m_policy->setParent(this);
  using namespace std;

  // Base group set-up
  auto allGroup = new Group{"All", Id<Group>{0}, &groupManager()};
  allGroup->addClient(m_policy->session()->localClient().id());
  groupManager().addGroup(allGroup);

  ISCORE_TODO;
  /*
    // Create it for each constraint / event.
    Scenario::ScenarioDocumentModel* bem = safe_cast<Scenario::ScenarioDocumentModel*>(&doc.model().modelDelegate());
    {
        ScenarioFindConstraintVisitor v;
        v.visit(bem->baseConstraint());// TODO this doesn't match baseconstraint
        for(Scenario::ConstraintModel* constraint : v.constraints)
        {
            for(const auto& plugid : elementPlugins())
            {
                if(constraint->pluginModelList.canAdd(plugid))
                    constraint->pluginModelList.add(
                                makeElementPlugin(constraint,
                                                  plugid,
                                                  constraint));
            }
        }
    }

    {
        ScenarioFindEventVisitor v;
        v.visit(bem->baseConstraint());

        for(Scenario::EventModel* event : v.events)
        {
            for(const auto& plugid : elementPlugins())
            {
                if(event->pluginModelList.canAdd(plugid))
                {
                    event->pluginModelList.add(
                                makeElementPlugin(event,
                                                  plugid,
                                                  event));
                }
            }
        }
    }
    // TODO here we have to instantiate Network "OSSIA" policies. OR does it go with GroupMetadata?
    */
}



void NetworkDocumentPlugin::serialize_impl(const VisitorVariant & vis) const
{
  ISCORE_TODO; // save uuid
  serialize_dyn(vis, *this);
}

auto NetworkDocumentPlugin::concreteKey() const -> ConcreteKey
{
  return DocumentPluginFactory::static_concreteKey();
}

void NetworkDocumentPlugin::setPolicy(NetworkPolicyInterface * pol)
{
  delete m_policy;
  m_policy = pol;

  emit sessionChanged();
}

iscore::DocumentPlugin*DocumentPluginFactory::load(
    const VisitorVariant& var,
    iscore::DocumentContext& doc,
    QObject* parent)
{
  return deserialize_dyn(var, [&] (auto&& deserializer)
  { return new NetworkDocumentPlugin{doc, deserializer, parent}; });
}

/*
std::vector<iscore::ElementPluginModelType> NetworkDocumentPlugin::elementPlugins() const
{
    return {GroupMetadata::staticPluginId()};
}

void NetworkDocumentPlugin::setupGroupPlugin(GroupMetadata* plug)
{
    connect(m_groups, &GroupManager::groupRemoved,
            plug, [=] (const Id<Group>& id)
    { if(plug->group() == id) plug->setGroup(m_groups->defaultGroup()); });
}



////// Methods relative to GroupMetadata //////
iscore::ElementPluginModel*
NetworkDocumentPlugin::makeElementPlugin(
        const QObject* element,
        iscore::ElementPluginModelType type,
        QObject* parent)
{
    qDebug() << element->metaObject()->className();
    switch(type)
    {
        case GroupMetadata::staticPluginId():
        {
            if(dynamic_cast<const Scenario::ConstraintModel*>(element) ||
               dynamic_cast<const Scenario::EventModel*>(element))
            {
                auto plug = new GroupMetadata{element, m_groups->defaultGroup(), parent};

                setupGroupPlugin(plug);

                return plug;
            }

            break;
        }
    }

    return nullptr;
}

iscore::ElementPluginModel*
NetworkDocumentPlugin::loadElementPlugin(
        const QObject* element,
        const VisitorVariant& vis,
        QObject* parent)
{
    if(dynamic_cast<const Scenario::ConstraintModel*>(element) ||
       dynamic_cast<const Scenario::EventModel*>(element))
    {
        auto plug = deserialize_dyn(vis, [&] (auto&& deserializer)
        { return new GroupMetadata{element, deserializer, parent}; });

        setupGroupPlugin(plug);

        return plug;
    }

    ISCORE_ABORT;
    return nullptr;
}


iscore::ElementPluginModel *NetworkDocumentPlugin::cloneElementPlugin(
        const QObject* element,
        iscore::ElementPluginModel* elt,
        QObject *parent)
{
    if(elt->elementPluginId() == GroupMetadata::staticPluginId())
    {
        auto newelt = static_cast<GroupMetadata*>(elt)->clone(element, parent);
        setupGroupPlugin(newelt);
        return newelt;
    }

    return nullptr;
}


////// Method relative to GRoupMetadataWidget //////
QWidget *NetworkDocumentPlugin::makeElementPluginWidget(
        const iscore::ElementPluginModel *var,
        QWidget* widg) const
{
    return new GroupMetadataWidget{
                static_cast<const GroupMetadata&>(*var), m_groups, widg};
}
*/
}

