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
      const iscore::DocumentContext& c);

private:
  Timekeeper& m_keep;
};
}
