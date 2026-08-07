#include "DeviceQueries.hpp"

#include <Device/Protocol/DeviceSettings.hpp>
#include <Device/Protocol/ProtocolFactoryInterface.hpp>
#include <Device/Protocol/ProtocolList.hpp>

#include <score/application/ApplicationContext.hpp>
#include <score/document/DocumentContext.hpp>
#include <score/serialization/JSONVisitor.hpp>

#include <QObject>

#include <Network/Communication/Rpc.hpp>

#include <memory>
#include <stdexcept>

namespace Network
{
namespace
{
const Device::ProtocolFactory&
requireProtocol(const score::DocumentContext& ctx, const rapidjson::Value& params)
{
  if(!params.IsObject() || !params.HasMember("protocol")
     || !params["protocol"].IsString())
    throw std::runtime_error{"a protocol must be named"};

  const QString uuid = QString::fromUtf8(
      params["protocol"].GetString(), params["protocol"].GetStringLength());

  auto& list = ctx.app.interfaces<Device::ProtocolFactoryList>();
  if(auto* factory = list.get(UuidKey<Device::ProtocolFactory>::fromString(uuid)))
    return *factory;

  throw std::runtime_error{
      QObject::tr("this machine has no protocol %1").arg(uuid).toStdString()};
}
}

void bindDeviceQueries(RpcChannel& rpc, const score::DocumentContext& ctx)
{
  // What this machine could make a device with. The capability exchange at join
  // already carries the uuids, but nothing a person can read: to offer a peer's
  // protocols in a list, the names and categories have to come across too.
  rpc.bind("device.protocols", [&ctx](const rapidjson::Value&) -> QByteArray {
    rapidjson::StringBuffer buf;
    JsonWriter w{buf};
    w.StartArray();
    for(auto& factory : ctx.app.interfaces<Device::ProtocolFactoryList>())
    {
      const auto uuid = score::uuids::toByteArray(factory.concreteKey().impl());
      const auto name = factory.prettyName().toUtf8();
      const auto category = factory.category().toUtf8();

      w.StartObject();
      w.Key("uuid");
      w.String(uuid.constData(), uuid.size());
      w.Key("name");
      w.String(name.constData(), name.size());
      w.Key("category");
      w.String(category.constData(), category.size());
      w.EndObject();
    }
    w.EndArray();
    return QByteArray{buf.GetString(), (int)buf.GetLength()};
  });

  // What is plugged into this machine, for one protocol. The settings travel
  // as the protocol wrote them: only this machine has to read them.
  rpc.bind("device.enumerate", [&ctx](const rapidjson::Value& params) -> QByteArray {
    const auto& factory = requireProtocol(ctx, params);

    // getEnumerators hands over ownership.
    std::vector<std::pair<QString, std::unique_ptr<Device::DeviceEnumerator>>> owned;
    for(auto [category, enumerator] : factory.getEnumerators(ctx))
      owned.emplace_back(category, std::unique_ptr<Device::DeviceEnumerator>{enumerator});

    rapidjson::StringBuffer buf;
    JsonWriter w{buf};
    w.StartArray();
    for(auto& [category, enumerator] : owned)
    {
      if(!enumerator)
        continue;

      const auto categoryUtf8 = category.toUtf8();
      enumerator->enumerate(
          [&](const QString& name, const Device::DeviceSettings& settings) {
        JSONReader settingsJson;
        settingsJson.readFrom(settings);
        const auto body = settingsJson.toByteArray();
        const auto nameUtf8 = name.toUtf8();

        w.StartObject();
        w.Key("category");
        w.String(categoryUtf8.constData(), categoryUtf8.size());
        w.Key("name");
        w.String(nameUtf8.constData(), nameUtf8.size());
        w.Key("settings");
        rapidjson::Document d;
        d.Parse(body.constData(), body.size());
        if(d.HasParseError())
          w.Null();
        else
          d.Accept(w);
        w.EndObject();
          });
    }
    w.EndArray();
    return QByteArray{buf.GetString(), (int)buf.GetLength()};
  });
}
}
