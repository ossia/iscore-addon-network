#include "DeviceValues.hpp"

#include <State/Message.hpp>

#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>

#include <score/document/DocumentContext.hpp>

#include <QObject>
#include <QTimer>

#include <core/application/ApplicationSettings.hpp>
#include <ossia/detail/hash_map.hpp>

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
namespace
{
//! Holds the latest value per address and sends them on a timer. A device that
//! reports at its own pace -- a mouse, an audio meter -- would otherwise put a
//! message on the socket per sample.
class ValueBroadcaster final : public QObject
{
public:
  ValueBroadcaster(Session& session, const score::DocumentContext& ctx, QObject* parent)
      : QObject{parent}
      , m_session{session}
  {
    auto* plug = ctx.findPlugin<Explorer::DeviceDocumentPlugin>();
    if(!plug)
      return;

    m_flush.setSingleShot(true);
    m_flush.setInterval(std::max(1, ctx.app.applicationSettings.uiEventRate));
    connect(&m_flush, &QTimer::timeout, this, &ValueBroadcaster::flush);

    plug->setValueObserver([this](const State::Address& addr, const ossia::value& v) {
      // Last one wins: a peer wants where the fader is, not every place it has
      // been since the last frame.
      m_pending[addr] = v;
      if(!m_flush.isActive())
        m_flush.start();
    });
  }

private:
  void flush()
  {
    auto pending = std::move(m_pending);
    m_pending.clear();

    for(auto& [addr, v] : pending)
    {
      m_session.broadcastToAllClients(m_session.makeMessage(
          MessagesAPI::instance().device_value_changed,
          State::Message{{addr, {}}, v}));
    }
  }

  Session& m_session;
  QTimer m_flush;
  ossia::hash_map<State::Address, ossia::value> m_pending;
};
}

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

void bindValueBroadcast(
    QObject& owner, Session& session, const score::DocumentContext& ctx)
{
  new ValueBroadcaster{session, ctx, &owner};
}

void bindValueDisplay(
    QObject& owner, Session& session, const score::DocumentContext& ctx)
{
  session.mapper().addHandler(
      &owner, MessagesAPI::instance().device_value_changed,
      [&ctx](const NetworkMessage& m) {
    auto* plug = ctx.findPlugin<Explorer::DeviceDocumentPlugin>();
    if(!plug)
      return;

    State::Message msg;
    if(!readingWireData("/device/value/changed", [&] {
         DataStreamWriter writer{m.data};
         writer.writeTo(msg);
       }))
      return;

    // Shown, not performed: this machine has no device to perform it on, and
    // asking for it would send it back where it came from.
    plug->updateProxy.updateLocalValue(msg.address, msg.value);
      });
}
}
