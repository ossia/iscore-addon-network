#include "DeviceStatus.hpp"

#include <Device/Protocol/DeviceInterface.hpp>

#include <Explorer/DeviceList.hpp>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>

#include <score/document/DocumentContext.hpp>
#include <score/tools/Bind.hpp>

#include <QObject>

#include <Network/Communication/MessageMapper.hpp>
#include <Network/Communication/Rpc.hpp>
#include <Network/Communication/WireJson.hpp>

#include <score/serialization/JSONVisitor.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <Network/Session/Session.hpp>

namespace Network
{
namespace
{
void report(Session& session, const Device::DeviceInterface& dev)
{
  // Kinds travel with the state, not only in the query answered at join: a
  // device plugged in after a peer arrived would otherwise be known to be
  // connected and not known to be a camera, so no combo box would offer it.
  session.broadcastToTerminals(session.makeMessage(
      MessagesAPI::instance().device_status, dev.settings().name, dev.connected(),
      (int)dev.kinds().toInt()));
}
}

void bindDeviceStatusBroadcast(
    QObject& owner, Session& session, const score::DocumentContext& ctx)
{
  auto* plug = ctx.findPlugin<Explorer::DeviceDocumentPlugin>();
  if(!plug)
    return;

  auto watch = [&owner, &session](Device::DeviceInterface& dev) {
    QObject::connect(
        &dev, &Device::DeviceInterface::connectionChanged, &owner,
        [&session, d = &dev](bool) { report(session, *d); });
  };

  plug->list().apply(watch);
  QObject::connect(
      &plug->list(), &Device::DeviceList::deviceAdded, &owner,
      [&session, watch](Device::DeviceInterface* dev) {
    if(!dev)
      return;
    watch(*dev);
    report(session, *dev);
      });
}

void broadcastAllDeviceStatus(Session& session, const score::DocumentContext& ctx)
{
  auto* plug = ctx.findPlugin<Explorer::DeviceDocumentPlugin>();
  if(!plug)
    return;

  plug->list().apply([&session](Device::DeviceInterface& dev) {
    report(session, dev);
  });
}

void bindDeviceStatusMirror(
    QObject& owner, Session& session, const score::DocumentContext& ctx)
{
  session.mapper().addHandler(
      &owner, MessagesAPI::instance().device_status,
      [&ctx](const NetworkMessage& m) {
    auto* plug = ctx.findPlugin<Explorer::DeviceDocumentPlugin>();
    if(!plug)
      return;

    QDataStream s{m.data};
    QString name;
    bool connected{};
    s >> name >> connected;
    plug->setRemoteConnected(name, connected);

    if(!s.atEnd())
    {
      int kinds{};
      s >> kinds;
      plug->setRemoteKinds(name, Device::DeviceKinds::fromInt(kinds));
    }
  });
}
}

namespace Network
{
void bindDeviceStatusQuery(RpcChannel& rpc, const score::DocumentContext& ctx)
{
  rpc.bind("device.statuses", [&ctx](const rapidjson::Value&) -> QByteArray {
    rapidjson::StringBuffer buf;
    JsonWriter w{buf};
    w.StartArray();

    if(auto* plug = ctx.findPlugin<Explorer::DeviceDocumentPlugin>())
    {
      plug->list().apply([&w](Device::DeviceInterface& dev) {
        const auto name = dev.settings().name.toUtf8();
        w.StartObject();
        w.Key("name");
        w.String(name.constData(), name.size());
        w.Key("connected");
        w.Bool(dev.connected());

        // What it can be plugged into. A peer with no device objects cannot
        // work this out for itself -- the answer used to be a cast to a
        // plug-in's own C++ type -- so it is told.
        w.Key("kinds");
        w.Int((int)dev.kinds().toInt());
        w.EndObject();
      });
    }

    w.EndArray();
    return QByteArray{buf.GetString(), (int)buf.GetLength()};
  });
}

void requestDeviceStatus(
    RpcChannel& rpc, const score::DocumentContext& ctx, const Id<Client>& peer)
{
  rpc.call(
      peer, "device.statuses", QByteArrayLiteral("{}"),
      [&ctx](const rapidjson::Value& result) {
    if(!result.IsArray())
      return;

    auto* plug = ctx.findPlugin<Explorer::DeviceDocumentPlugin>();
    if(!plug)
      return;

    for(const auto& e : result.GetArray())
    {
      const auto name = wireString(e, "name");
      const auto connected = wireBool(e, "connected");
      if(!name || !connected)
        continue;

      plug->setRemoteConnected(*name, *connected);

      if(const auto kinds = wireInt(e, "kinds"))
        plug->setRemoteKinds(*name, Device::DeviceKinds::fromInt((int)*kinds));
    }
      },
      [](const QString& err) {
    qDebug() << "Could not read the other machine's device states:" << err;
      });
}
}
