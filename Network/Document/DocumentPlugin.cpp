#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>
#include <Scenario/Process/ScenarioModel.hpp>
#include <iscore/tools/std/Optional.hpp>
#include <core/document/Document.hpp>
#include <core/document/DocumentModel.hpp>
#include <iscore/serialization/VisitorCommon.hpp>

#include <QString>

#include <Network/Group/Group.hpp>
#include <Network/Group/GroupManager.hpp>
#include <Network/Group/GroupMetadata.hpp>
#include <Network/Group/GroupMetadataWidget.hpp>
#include "DocumentPlugin.hpp"
#include <Process/Process.hpp>
#include <Network/Session/Session.hpp>
#include <Scenario/Document/Constraint/ConstraintModel.hpp>
#include <Scenario/Document/Event/EventModel.hpp>
#include <iscore/plugins/documentdelegate/plugin/DocumentPlugin.hpp>
#include <iscore/model/EntityMap.hpp>
#include <iscore/model/Identifier.hpp>
#include <Network/Client/LocalClient.hpp>
#include <Network/Group/Panel/GroupPanelDelegate.hpp>
class QWidget;
struct VisitorVariant;

namespace Network
{
NetworkDocumentPlugin::NetworkDocumentPlugin(
    const iscore::DocumentContext& ctx,
    NetworkPolicy *policy,
    Id<iscore::DocumentPlugin> id,
    QObject* parent):
  iscore::SerializableDocumentPlugin{ctx, std::move(id), "NetworkDocumentPlugin", parent},
  m_policy{policy},
  m_groups{new GroupManager{this}}
{
  m_policy->setParent(this);

  // Base group set-up
  auto allGroup = new Group{"all", Id<Group>{0}, &groupManager()};
  allGroup->addClient(m_policy->session()->localClient().id());
  groupManager().addGroup(allGroup);
}

void NetworkDocumentPlugin::setPolicy(NetworkPolicy * pol)
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

}

