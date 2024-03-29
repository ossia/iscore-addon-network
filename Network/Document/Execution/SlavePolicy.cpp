#include <score/model/path/PathSerialization.hpp>

#include <Network/Communication/MessageMapper.hpp>
#include <Network/Document/Execution/SlavePolicy.hpp>
#include <Network/Session/MasterSession.hpp>

namespace Network
{

SlaveExecutionPolicy::SlaveExecutionPolicy(
    Session& s, NetworkDocumentPlugin& doc, const score::DocumentContext& c)
    : m_session{s}
{
  qDebug("SlaveExecutionPolicy");
  auto& mapi = MessagesAPI::instance();
  s.mapper().addHandler_(
      mapi.trigger_entered,
      [&](const NetworkMessage& m, Path<Scenario::TimeSyncModel> p) {
    auto it = doc.noncompensated.trigger_evaluation_entered.find(p);
    if(it != doc.noncompensated.trigger_evaluation_entered.end())
    {
      if(it->second)
        it->second(m.clientId);
    }
      });

  s.mapper().addHandler_(
      mapi.trigger_left,
      [&](const NetworkMessage& m, Path<Scenario::TimeSyncModel> p) {});

  s.mapper().addHandler_(
      mapi.trigger_finished,
      [&](const NetworkMessage& m, Path<Scenario::TimeSyncModel> p, bool val) {
    auto it = doc.noncompensated.trigger_evaluation_finished.find(p);
    if(it != doc.noncompensated.trigger_evaluation_finished.end())
    {
      if(it->second)
        it->second(m.clientId, val);
    }
      });

  s.mapper().addHandler_(
      mapi.trigger_triggered,
      [&](const NetworkMessage& m, Path<Scenario::TimeSyncModel> p, bool val) {
    auto it = doc.noncompensated.trigger_triggered.find(p);
    if(it != doc.noncompensated.trigger_triggered.end())
    {
      if(it->second)
        it->second(m.clientId);
    }
      });
  s.mapper().addHandler_(
      mapi.trigger_triggered_compensated,
      [&](const NetworkMessage& m, Path<Scenario::TimeSyncModel> p, qint64 ns,
          bool val) {
    auto it = doc.compensated.trigger_triggered.find(p);
    if(it != doc.compensated.trigger_triggered.end())
    {
      if(it->second)
        it->second(m.clientId, ns);
    }
      });

  s.mapper().addHandler_(
      mapi.interval_speed,
      [&](const NetworkMessage& m, Path<Scenario::IntervalModel> p, double val) {
    auto it = doc.noncompensated.interval_speed_changed.find(p);
    if(it != doc.noncompensated.interval_speed_changed.end())
    {
      if(it->second)
        it->second(m.clientId, val);
    }
      });

  s.mapper().addHandler_(
      mapi.netpit_out_message,
      [&](const NetworkMessage& m, uint64_t process,
          std::vector<std::pair<Id<Client>, ossia::value>> vec) {
    // Apply to the local process
    this->on_message(process, std::move(vec));
      });

  s.mapper().addHandler_(
      mapi.netpit_out_audio,
      [&](const NetworkMessage& m, uint64_t process,
          std::vector<std::pair<Id<Client>, std::vector<std::vector<float>>>> vec) {
    // Apply to the local process
    this->on_audio(process, std::move(vec));
      });

  s.mapper().addHandler_(
      mapi.netpit_out_video,
      [&](const NetworkMessage& m, uint64_t process, QByteArray vec) {
    // Apply to the local process
    this->on_video(process, Netpit::InboundImage{vec, (int)m.clientId.val()});
      });
}

void SlaveExecutionPolicy::writeMessage(Netpit::OutboundMessage m)
{
  auto& mapi = MessagesAPI::instance();
  m_session.sendMessage(
      m_session.master().id(),
      m_session.makeMessage(mapi.netpit_in_message, m.instance, m.val));
}

void SlaveExecutionPolicy::writeAudio(Netpit::OutboundAudio&& m)
{
  auto& mapi = MessagesAPI::instance();
  m_session.sendMessage(
      m_session.master().id(),
      m_session.makeMessage(mapi.netpit_in_audio, m.instance, m.channels));
}

void SlaveExecutionPolicy::writeVideo(Netpit::OutboundImage&& m)
{
  auto& mapi = MessagesAPI::instance();
  m_session.sendMessage(
      m_session.master().id(),
      m_session.makeMessage(mapi.netpit_in_video, m.instance, m.texture));
}
}
