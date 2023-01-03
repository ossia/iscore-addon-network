#pragma once
#include <ossia/detail/flat_map.hpp>

#include <Network/Document/DocumentPlugin.hpp>

#include <unordered_map>

namespace Network
{
struct Timekeeper;
class MasterExecutionPolicy : public ExecutionPolicy
{
public:
  MasterExecutionPolicy(
      Session& s, NetworkDocumentPlugin& doc, const score::DocumentContext& c);

  void writeMessage(Netpit::OutboundMessage m) override;
  void writeAudio(Netpit::OutboundAudio&& m) override;
  void writeVideo(Netpit::OutboundImage&& m) override;

private:
  Session& m_session;
  Timekeeper& m_keep;

  // For each "Netpit" process we keep a list of what each client sent as value, if any
  std::unordered_map<int64_t, ossia::flat_map<Id<Client>, ossia::value>> m_messages;
  std::unordered_map<
      int64_t, ossia::flat_map<Id<Client>, std::vector<std::vector<float>>>>
      m_audios;
  std::unordered_map<int64_t, ossia::flat_map<Id<Client>, QByteArray>> m_videos;
};
}
