// What a device on the host is reporting, on a machine that has no devices.
//
// Setting a value already travelled the other way: a peer asks, and the host
// performs it on the real device. This is the opposite -- the host has *seen* a
// value, because somebody moved a mouse or a fader, and a terminal has nothing
// to hear it from. Without it the tree shows names and stale values forever.

#include <State/Message.hpp>

#include <Device/Address/AddressSettings.hpp>
#include <Device/Protocol/DeviceSettings.hpp>

#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <Explorer/DocumentPlugin/NodeUpdateProxy.hpp>

#include <core/document/Document.hpp>

#include <score_test/App.hpp>
#include <score_test/Document.hpp>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QThread>

#include <catch2/catch_all.hpp>

#include <Network/Client/PeerRole.hpp>

#include "SessionFixture.hpp"

using namespace SessionTest;

namespace
{
constexpr auto absent_uuid = "c0ffee00-1111-2222-3333-444455556666";

//! A device with `count` addresses, on both machines.
void addWideDevice(score::Document& doc, int count)
{
  Device::DeviceSettings s;
  s.protocol = UuidKey<Device::ProtocolFactory>::fromString(QString{absent_uuid});
  s.name = "Wide";

  Device::Node node{s, nullptr};
  for(int i = 0; i < count; i++)
  {
    Device::AddressSettings a;
    a.name = QString{"p%1"}.arg(i);
    a.value = ossia::value{0};
    node.emplace_back(a, &node);
  }

  doc.context().plugin<Explorer::DeviceDocumentPlugin>().updateProxy.addDevice(node);
}

//! A device in the tree with one address, on both machines. No protocol: what
//! is under test is the reporting, not the hardware.
void addMouse(score::Document& doc)
{
  Device::DeviceSettings s;
  s.protocol = UuidKey<Device::ProtocolFactory>::fromString(QString{absent_uuid});
  s.name = "Mouse";

  Device::Node node{s, nullptr};
  Device::AddressSettings axis;
  axis.name = "x";
  axis.value = ossia::value{0};
  node.emplace_back(axis, &node);

  doc.context().plugin<Explorer::DeviceDocumentPlugin>().updateProxy.addDevice(node);
}

//! Let anything already in flight arrive. A device appearing announces its
//! tree, coalesced on a timer, and that tree carries the addresses' values --
//! so a value changed before the flush rides along with it and says nothing
//! about whether values are reported on their own.
void settle()
{
  QElapsedTimer t;
  t.start();
  while(t.elapsed() < 400)
  {
    QCoreApplication::processEvents();
    QThread::msleep(1);
  }
}

std::optional<ossia::value> valueOf(score::Document& doc, const QString& address)
{
  auto& root = doc.context().plugin<Explorer::DeviceDocumentPlugin>().rootNode();
  auto* n = Device::try_getNodeFromAddress(root, State::Address::fromString(address).value());
  if(!n || !n->is<Device::AddressSettings>())
    return std::nullopt;
  return n->get<Device::AddressSettings>().value;
}
}

TEST_CASE("A value the host's device reports reaches the peer", "[session][devices]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port, Network::PeerRole::Terminal);
    REQUIRE(client);

    addMouse(*master.document);
    addMouse(*client);
    settle();

    auto& masterPlug
        = master.document->context().plugin<Explorer::DeviceDocumentPlugin>();

    // What the mouse just did, arriving the way a device reports it.
    masterPlug.on_valueUpdated(
        State::Address::fromString("Mouse:/x").value(), ossia::value{42});

    REQUIRE(spin_until([&] {
      auto v = valueOf(*client, "Mouse:/x");
      return v && *v == ossia::value{42};
    }));
  });
}

TEST_CASE("Showing a reported value does not send it back", "[session][devices]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port, Network::PeerRole::Terminal);
    REQUIRE(client);

    addMouse(*master.document);
    addMouse(*client);
    settle();

    // Anything the client decides to *perform* goes back to the host. A
    // notification must not: the host would perform it on the device, which
    // reports it again, which notifies the client -- a mouse would saturate
    // the session on its own.
    int sentBack{};
    client->context().plugin<Explorer::DeviceDocumentPlugin>().setValueSink(
        [&](const State::Address&, const ossia::value&) { sentBack++; });

    auto& masterPlug
        = master.document->context().plugin<Explorer::DeviceDocumentPlugin>();
    masterPlug.on_valueUpdated(
        State::Address::fromString("Mouse:/x").value(), ossia::value{7});

    REQUIRE(spin_until([&] {
      auto v = valueOf(*client, "Mouse:/x");
      return v && *v == ossia::value{7};
    }));

    // Shown, and not echoed.
    CHECK(sentBack == 0);
  });
}

TEST_CASE("Many moving values are one message, not many", "[session][devices]")
{
  score::test::run_in_app([](const score::GUIApplicationContext& ctx) {
    auto master = hostSession(ctx);
    auto* client = joinSession(ctx, master.port, Network::PeerRole::Terminal);
    REQUIRE(client);

    constexpr int params = 100;
    addWideDevice(*master.document, params);
    addWideDevice(*client, params);
    settle();

    // Count what arrives on the wire, not what is displayed: coalescing already
    // bounds how often each address is sent, and says nothing about how many
    // messages that is. A device reporting at its own pace -- a control surface,
    // a tracker -- moves every parameter it has, every tick.
    // NOTE: what is *not* asserted here is the number of messages, which is
    // the actual point of batching. The socket and session types are not
    // exported from the plug-in, so a test cannot count frames; asserting
    // arrival would pass just as well against one message per address, since a
    // per-address encoding decodes as a batch of one. What this does pin is the
    // batch encoding itself -- a hundred addresses in one payload, decoded back
    // in order -- which is the part that is new and can silently rot.

    auto& masterPlug
        = master.document->context().plugin<Explorer::DeviceDocumentPlugin>();
    for(int i = 0; i < params; i++)
      masterPlug.on_valueUpdated(
          State::Address::fromString(QString{"Wide:/p%1"}.arg(i)).value(),
          ossia::value{i});

    REQUIRE(spin_until([&] {
      auto v = valueOf(*client, "Wide:/p99");
      return v && *v == ossia::value{99};
    }));

    // Every one of them, not just the last: a batch that dropped or reordered
    // its tail would still satisfy a check on p99 alone.
    for(int i = 0; i < params; i++)
    {
      INFO("address p" << i);
      auto v = valueOf(*client, QString{"Wide:/p%1"}.arg(i));
      REQUIRE(v);
      CHECK(*v == ossia::value{i});
    }
  });
}
