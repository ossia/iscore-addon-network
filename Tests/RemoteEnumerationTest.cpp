// What is plugged into the other machine.
//
// A terminal cannot enumerate hardware: it has none of it, and often not even
// the protocol. So the Add-device dialog asks the host, and what comes back is
// the only way to add such a device from there. When that list is empty the
// device simply cannot be added.

#include <Device/Protocol/ProtocolFactoryInterface.hpp>
#include <Device/Protocol/ProtocolList.hpp>

#include <score/plugins/InterfaceList.hpp>

#include <core/document/Document.hpp>

#include <score_test/App.hpp>
#include <score_test/Document.hpp>

#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Document/RemoteDeviceCatalog.hpp>

#include <catch2/catch_all.hpp>

#include "SessionFixture.hpp"

using namespace SessionTest;

namespace
{
//! A protocol whose devices this machine can actually list, so the test says
//! something about the wire rather than about the hardware.
struct Enumerable
{
  UuidKey<Device::ProtocolFactory> key;
  QString name;
  int count{};
};

//! Every protocol whose devices this machine can list. All of them, not the
//! first: a wire that carries one protocol's list and drops another's looks
//! perfectly healthy from a single sample.
std::vector<Enumerable>
allEnumerable(const score::GUIApplicationContext& ctx, score::Document& doc)
{
  std::vector<Enumerable> out;
  for(auto& factory : ctx.interfaces<Device::ProtocolFactoryList>())
  {
    int n = 0;
    for(auto [category, enumerator] : factory.getEnumerators(doc.context()))
    {
      std::unique_ptr<Device::DeviceEnumerator> owned{enumerator};
      if(owned)
        owned->enumerate([&](const QString&, const Device::DeviceSettings&) { n++; });
    }
    if(n > 0)
      out.push_back(Enumerable{factory.concreteKey(), factory.prettyName(), n});
  }
  return out;
}
}

TEST_CASE("A peer lists what is plugged into the other machine", "[session][devices]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    REQUIRE(master.port > 0);

    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    const auto enumerable = allEnumerable(ctx, *master.document);
    if(enumerable.empty())
    {
      WARN("no protocol on this machine enumerates anything; nothing to check");
      return;
    }

    auto* plug = client->context().findPlugin<Network::NetworkDocumentPlugin>();
    REQUIRE(plug);
    auto* rpc = plug->rpc();
    REQUIRE(rpc);

    Network::RemoteDeviceCatalog catalog{*rpc, master.session->localClient().id(), nullptr};

    for(const Enumerable& proto : enumerable)
    {
      INFO("protocol: " << proto.name.toStdString());

      std::vector<QString> got;
      catalog.enumerate(
          proto.key,
          [&](const QString&, const QString& name, const Device::DeviceSettings&) {
        got.push_back(name);
          });

      // The answer crosses the session, so it is not there yet.
      CHECK(spin_until([&] { return got.size() == (std::size_t)proto.count; }));
      CHECK(got.size() == (std::size_t)proto.count);
    }
  });
}

TEST_CASE("The protocols offered are the other machine's", "[session][devices]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port);
    REQUIRE(client);

    auto* plug = client->context().findPlugin<Network::NetworkDocumentPlugin>();
    REQUIRE(plug);
    auto* rpc = plug->rpc();
    REQUIRE(rpc);

    Network::RemoteDeviceCatalog catalog{*rpc, master.session->localClient().id(), nullptr};
    REQUIRE(spin_until([&] { return !catalog.protocols().empty(); }));

    // A device is added on the machine that has the protocol, so the list has
    // to be that machine's -- including protocols this one cannot construct.
    const auto& local = ctx.interfaces<Device::ProtocolFactoryList>();
    CHECK(catalog.protocols().size() == (std::size_t)local.size());
  });
}
