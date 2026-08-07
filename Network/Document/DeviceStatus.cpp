#include "DeviceStatus.hpp"

#include <Device/Protocol/DeviceInterface.hpp>

#include <Explorer/DeviceList.hpp>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>

#include <score/document/DocumentContext.hpp>
#include <score/tools/Bind.hpp>

#include <QObject>

#include <Network/Communication/MessageMapper.hpp>
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
