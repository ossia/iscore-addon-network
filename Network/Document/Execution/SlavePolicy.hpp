#pragma once
#include <Network/Document/DocumentPlugin.hpp>

namespace Network
{
class SlaveExecutionPolicy : public ExecutionPolicy
{
public:
  SlaveExecutionPolicy(
      Session& s,
      NetworkDocumentPlugin& doc,
      const score::DocumentContext& c);

  void writeMessage(Netpit::Message m);

private:
  Session& m_session;
};
}
