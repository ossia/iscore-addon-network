#pragma once
#include <Network/Document/DocumentPlugin.hpp>

namespace Network
{
class MasterExecutionPolicy : public ExecutionPolicy
{
public:
  MasterExecutionPolicy(
      Session& s,
      NetworkDocumentPlugin& doc,
      const iscore::DocumentContext& c);
};
}
