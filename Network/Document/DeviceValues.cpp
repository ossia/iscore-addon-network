#include "DeviceValues.hpp"

#include <State/Message.hpp>

#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>

#include <score/document/DocumentContext.hpp>

#include <QObject>

#include <Network/Communication/MessageMapper.hpp>
#include <Network/Communication/WireRead.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <Network/Session/Session.hpp>
#include <Network/Session/ClientSession.hpp>
#include <Network/Client/RemoteClient.hpp>

#include <State/MessageListSerialization.hpp>
#include <score/serialization/DataStreamVisitor.hpp>

namespace Network
{
void bindValueForwarding(ClientSession& session, const score::DocumentContext& ctx)
{
  auto* plug = ctx.findPlugin<Explorer::DeviceDocumentPlugin>();
  if(!plug)
    return;

  plug->setValueSink([&session](const State::Address& addr, const ossia::value& v) {
    session.master().sendMessage(session.makeMessage(
        MessagesAPI::instance().device_value, State::Message{{addr, {}}, v}));
  });
}

void bindValueSetter(
    QObject& owner, Session& session, const score::DocumentContext& ctx)
{
  session.mapper().addHandler(
      &owner, MessagesAPI::instance().device_value, [&ctx](const NetworkMessage& m) {
    auto* plug = ctx.findPlugin<Explorer::DeviceDocumentPlugin>();
    if(!plug)
      return;

    State::Message msg;
    if(!readingWireData("/device/value", [&] {
         DataStreamWriter writer{m.data};
         writer.writeTo(msg);
       }))
      return;

    // The same call the device explorer makes here: a peer's edit is an edit,
    // and everything downstream of it -- the protocol, the OSC packet -- is
    // this machine's to do.
    plug->updateProxy.updateRemoteValue(msg.address.address, msg.value);
      });
}
}
