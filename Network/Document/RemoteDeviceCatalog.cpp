#include "RemoteDeviceCatalog.hpp"

#include <Device/Protocol/DeviceSettings.hpp>

#include <score/application/ApplicationContext.hpp>
#include <score/plugins/InterfaceList.hpp>
#include <score/serialization/JSONVisitor.hpp>

#include <Device/Protocol/ProtocolFactoryInterface.hpp>
#include <Device/Protocol/ProtocolList.hpp>

#include <Network/Communication/Rpc.hpp>
#include <Network/Communication/WireJson.hpp>

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
      const auto uuid = wireString(e, "uuid");
      const auto name = wireString(e, "name");
      if(!uuid || !name)
        continue;

      Protocol p;
      p.key = UuidKey<Device::ProtocolFactory>::fromString(*uuid);
      p.name = *name;
      p.category = wireString(e, "category").value_or(QString{});
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
      const auto name = wireString(e, "name");
      const auto* settingsValue = wireMember(e, "settings");
      if(!name || !settingsValue)
        continue;

      // As the protocol wrote them: held verbatim and handed back in the
      // command, where the machine that has the protocol reads them.
      Device::DeviceSettings settings;
      {
        JSONObject::Deserializer des{*settingsValue};
        des.writeTo(settings);
      }

      onDevice(wireString(e, "category").value_or(QString{}), *name, settings);
    }
      },
      [](const QString& err) {
    qDebug() << "Could not list the other machine's devices:" << err;
      });
}
}
