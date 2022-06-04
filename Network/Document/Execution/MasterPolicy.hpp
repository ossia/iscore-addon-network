#pragma once
#include <Network/Document/DocumentPlugin.hpp>

namespace Network
{
struct Timekeeper;
class MasterExecutionPolicy : public ExecutionPolicy
{
public:
  MasterExecutionPolicy(
      Session& s,
      NetworkDocumentPlugin& doc,
      const score::DocumentContext& c);

  void writeMessage(Netpit::Message m) override;
private:
  Session& m_session;
  Timekeeper& m_keep;

  // For each "Netpit" process we keep a list of what each client sent as value, if any
  std::unordered_map<int64_t, ossia::flat_map<Id<Client>, ossia::value>> m_messages;
};
}
