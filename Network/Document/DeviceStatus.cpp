#include "DeviceStatus.hpp"

#include <Device/Protocol/DeviceInterface.hpp>

#include <Explorer/DeviceList.hpp>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>

#include <score/document/DocumentContext.hpp>
#include <score/tools/Bind.hpp>

#include <QObject>

#include <Network/Communication/MessageMapper.hpp>
#include <Network/Communication/Rpc.hpp>

#include <score/serialization/JSONVisitor.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <Network/Session/Session.hpp>

namespace Network
{
namespace
{
void report(Session& session, const QString& name, bool connected)
{
  session.broadcastToAllClients(
      session.makeMessage(MessagesAPI::instance().device_status, name, connected));
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
        [&session, name = dev.settings().name](bool connected) {
      report(session, name, connected);
        });
  };

  plug->list().apply(watch);
  QObject::connect(
      &plug->list(), &Device::DeviceList::deviceAdded, &owner,
      [&session, watch](Device::DeviceInterface* dev) {
    if(!dev)
      return;
    watch(*dev);
    report(session, dev->settings().name, dev->connected());
      });
}

void broadcastAllDeviceStatus(Session& session, const score::DocumentContext& ctx)
{
  auto* plug = ctx.findPlugin<Explorer::DeviceDocumentPlugin>();
  if(!plug)
    return;

  plug->list().apply([&session](Device::DeviceInterface& dev) {
    report(session, dev.settings().name, dev.connected());
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
      if(!e.IsObject() || !e.HasMember("name") || !e.HasMember("connected"))
        continue;

      const auto name
          = QString::fromUtf8(e["name"].GetString(), e["name"].GetStringLength());
      plug->setRemoteConnected(name, e["connected"].GetBool());

      if(e.HasMember("kinds"))
        plug->setRemoteKinds(
            name, Device::DeviceKinds::fromInt(e["kinds"].GetInt()));
    }
      },
      [](const QString& err) {
    qDebug() << "Could not read the other machine's device states:" << err;
      });
}
}
