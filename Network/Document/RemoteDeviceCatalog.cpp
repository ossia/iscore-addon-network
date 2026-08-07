#include "RemoteDeviceCatalog.hpp"

#include <Device/Protocol/DeviceSettings.hpp>

#include <score/application/ApplicationContext.hpp>
#include <score/plugins/InterfaceList.hpp>
#include <score/serialization/JSONVisitor.hpp>

#include <Device/Protocol/ProtocolFactoryInterface.hpp>
#include <Device/Protocol/ProtocolList.hpp>

#include <Network/Communication/Rpc.hpp>

namespace Network
{
RemoteDeviceCatalog::RemoteDeviceCatalog(
    RpcChannel& rpc, Id<Client> peer, QObject* parent)
    : QObject{parent}
    , m_rpc{&rpc}
    , m_peer{std::move(peer)}
{
  // Asked once, on the way in, so that the list is there when someone opens the
  // dialog rather than arriving while they read it.
  m_rpc->call(
      m_peer, "device.protocols", QByteArrayLiteral("{}"),
      [this](const rapidjson::Value& result) {
    if(!result.IsArray())
      return;

    auto& local = score::AppContext().interfaces<Device::ProtocolFactoryList>();
    std::vector<Protocol> found;
    for(const auto& e : result.GetArray())
    {
      if(!e.IsObject() || !e.HasMember("uuid") || !e.HasMember("name"))
        continue;

      Protocol p;
      p.key = UuidKey<Device::ProtocolFactory>::fromString(QString::fromUtf8(
          e["uuid"].GetString(), e["uuid"].GetStringLength()));
      p.name = QString::fromUtf8(e["name"].GetString(), e["name"].GetStringLength());
      p.category = e.HasMember("category")
                       ? QString::fromUtf8(
                             e["category"].GetString(), e["category"].GetStringLength())
                       : QString{};
      p.constructible = local.get(p.key) != nullptr;
      found.push_back(std::move(p));
    }

    m_protocols = std::move(found);
      },
      [](const QString& err) {
    qDebug() << "Could not list the other machine's protocols:" << err;
      });
}

RemoteDeviceCatalog::~RemoteDeviceCatalog() = default;

std::vector<Device::DeviceCatalog::Protocol> RemoteDeviceCatalog::protocols() const
{
  return m_protocols;
}

void RemoteDeviceCatalog::enumerate(
    const UuidKey<Device::ProtocolFactory>& protocol, OnDevice onDevice)
{
  if(!m_rpc || !onDevice)
    return;

  const auto uuid = score::uuids::toByteArray(protocol.impl());

  rapidjson::StringBuffer buf;
  JsonWriter w{buf};
  w.StartObject();
  w.Key("protocol");
  w.String(uuid.constData(), uuid.size());
  w.EndObject();

  m_rpc->call(
      m_peer, "device.enumerate", QByteArray{buf.GetString(), (int)buf.GetLength()},
      [onDevice = std::move(onDevice)](const rapidjson::Value& result) {
    if(!result.IsArray())
      return;

    for(const auto& e : result.GetArray())
    {
      if(!e.IsObject() || !e.HasMember("name") || !e.HasMember("settings"))
        continue;

      // The settings are as the protocol wrote them, which nothing here can
      // parse -- and does not need to. They are held as they are and handed
      // back in the command that creates the device, where the machine that
      // has the protocol reads them.
      Device::DeviceSettings settings;
      {
        JSONObject::Deserializer des{e["settings"]};
        des.writeTo(settings);
      }

      QString label = QString::fromUtf8(
          e["name"].GetString(), e["name"].GetStringLength());
      if(e.HasMember("category") && e["category"].GetStringLength() > 0)
        label = QStringLiteral("%1 / %2")
                    .arg(QString::fromUtf8(
                        e["category"].GetString(), e["category"].GetStringLength()))
                    .arg(label);

      onDevice(label, settings);
    }
      },
      [](const QString& err) {
    qDebug() << "Could not list the other machine's devices:" << err;
      });
}
}
