#include "DeviceTree.hpp"

#include <Device/Node/DeviceNode.hpp>
#include <Device/Protocol/DeviceSettings.hpp>

#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <Explorer/Explorer/DeviceExplorerModel.hpp>

#include <score/application/ApplicationContext.hpp>
#include <score/document/DocumentContext.hpp>
#include <score/model/tree/TreeNodeSerialization.hpp>
#include <score/serialization/JSONVisitor.hpp>

#include <core/application/ApplicationSettings.hpp>

#include <ossia/detail/hash_map.hpp>

#include <QObject>
#include <QTimer>

#include <Network/Communication/MessageMapper.hpp>
#include <Network/Communication/WireRead.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <Network/Session/Session.hpp>

namespace Network
{
namespace
{
const Device::Node*
findDeviceNode(const Explorer::DeviceDocumentPlugin& plug, const QString& name)
{
  for(const auto& n : plug.rootNode())
    if(n.is<Device::DeviceSettings>() && n.get<Device::DeviceSettings>().name == name)
      return &n;
  return nullptr;
}

//! Sends a device's tree when it changes, coalesced: a refresh arrives as many
//! insertions, and sending on each would send the tree once per node found.
class DeviceTreeBroadcaster final : public QObject
{
public:
  DeviceTreeBroadcaster(
      Session& session, const score::DocumentContext& ctx, QObject* parent)
      : QObject{parent}
      , m_session{session}
      , m_ctx{ctx}
  {
    auto* plug = ctx.findPlugin<Explorer::DeviceDocumentPlugin>();
    if(!plug)
      return;

    m_flush.setSingleShot(true);
    m_flush.setInterval(std::max(1, ctx.app.applicationSettings.uiEventRate));
    connect(&m_flush, &QTimer::timeout, this, &DeviceTreeBroadcaster::flush);

    connect(
        plug, &Explorer::DeviceDocumentPlugin::deviceTreeChanged, this,
        [this](const QString& device) {
      m_dirty.insert(device);
      if(!m_flush.isActive())
        m_flush.start();
        });
  }

private:
  void flush()
  {
    // Nobody is watching: the whole point of this machinery is a peer that
    // does not run the score, and a session may have none.
    if(!m_session.hasTerminals())
    {
      m_dirty.clear();
      return;
    }

    auto* plug = m_ctx.findPlugin<Explorer::DeviceDocumentPlugin>();
    if(!plug)
      return;

    for(const auto& name : m_dirty)
    {
      const auto* node = findDeviceNode(*plug, name);
      if(!node)
        continue;

      JSONReader r;
      r.readFrom(*node);
      m_session.broadcastToTerminals(m_session.makeMessage(
          MessagesAPI::instance().device_tree, name, r.toByteArray()));
    }
    m_dirty.clear();
  }

  Session& m_session;
  const score::DocumentContext& m_ctx;
  ossia::hash_set<QString> m_dirty;
  QTimer m_flush;
};
}

void bindDeviceTreeBroadcast(
    QObject& owner, Session& session, const score::DocumentContext& ctx)
{
  new DeviceTreeBroadcaster{session, ctx, &owner};
}

void bindDeviceTreeMirror(
    QObject& owner, Session& session, const score::DocumentContext& ctx)
{
  session.mapper().addHandler(
      &owner, MessagesAPI::instance().device_tree, [&ctx](const NetworkMessage& m) {
    auto* plug = ctx.findPlugin<Explorer::DeviceDocumentPlugin>();
    if(!plug)
      return;

    QString name;
    QByteArray nodeJson;
    if(!readingWireData("/device/tree", [&] {
         QDataStream s{m.data};
         s >> name >> nodeJson;
       }))
      return;

    if(name.isEmpty())
      return;

    auto doc = readJson(nodeJson);
    if(doc.HasParseError() || !doc.IsObject())
      return;

    // Inside the guard too: this is where a peer's bytes reach the device and
    // protocol deserializers, which are written for our own save files and
    // assert their way through anything else -- and rapidjson's assertions are
    // compiled out in release, so a missing member is a wild read rather than
    // a stop.
    Device::Node node;
    if(!readingWireData("/device/tree", [&] {
         JSONObject::Deserializer des{doc};
         des.writeTo(node);
       }))
      return;

    // Named by the sender and by itself: a mismatch would replace one device
    // with another's tree.
    if(!node.is<Device::DeviceSettings>()
       || node.get<Device::DeviceSettings>().name != name)
      return;

    plug->explorer().replaceDevice(name, node);
      });
}
}
