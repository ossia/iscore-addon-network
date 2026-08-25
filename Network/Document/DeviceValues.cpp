#include "DeviceValues.hpp"

#include <State/Message.hpp>

#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>

#include <score/document/DocumentContext.hpp>

#include <QObject>
#include <QDataStream>
#include <QTimer>

#include <stdexcept>
#include <vector>

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
    // Nobody is watching: the whole point of this machinery is a peer that
    // does not run the score, and a session may have none.
    if(!m_session.hasTerminals())
    {
      m_pending.clear();
      return;
    }

    auto pending = std::move(m_pending);
    m_pending.clear();

    // One message for the batch, as the transport does. Coalescing bounds how
    // often each address is sent; it does nothing about how many messages that
    // is, and a device tree with a few hundred moving parameters was sending a
    // few hundred frames per flush, per watching peer.
    QByteArray payload;
    QDataStream s{&payload, QIODevice::WriteOnly};
    qint32 count = 0;

    for(auto& [addr, v] : pending)
    {
      s << score::marshall<DataStream>(State::Message{{addr, {}}, v});
      count++;
    }

    m_session.broadcastToTerminals(m_session.makeMessage(
        MessagesAPI::instance().device_value_changed, count, payload));
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

    std::vector<State::Message> batch;
    if(!readingWireData("/device/value/changed", [&] {
         QDataStream s{m.data};
         qint32 count{};
         QByteArray payload;
         s >> count >> payload;
         if(s.status() != QDataStream::Ok || count < 0)
           throw std::runtime_error{"malformed value batch"};

         QDataStream ps{payload};
         batch.reserve(std::min(count, qint32(4096)));
         for(qint32 i = 0; i < count; i++)
         {
           QByteArray one;
           ps >> one;
           if(ps.status() != QDataStream::Ok)
             throw std::runtime_error{"short value batch"};

           State::Message msg;
           DataStreamWriter writer{one};
           writer.writeTo(msg);
           batch.push_back(std::move(msg));
         }
       }))
      return;

    // Shown, not performed: this machine has no device to perform it on, and
    // asking for it would send it back where it came from.
    for(const auto& msg : batch)
      plug->updateProxy.updateLocalValue(msg.address, msg.value);
      });
}
}
