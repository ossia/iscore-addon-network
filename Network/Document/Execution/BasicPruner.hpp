#pragma once
#include <iscore/model/Identifier.hpp>
namespace Engine
{
namespace Execution
{
class ConstraintComponent;
class TimeNodeComponent;
class Context;
}
}
namespace Scenario
{
class ScenarioInterface;
}
namespace Network
{
class Group;
class Client;
class NetworkDocumentPlugin;

struct BasicPruner
{
  BasicPruner(NetworkDocumentPlugin& d);

  void recurse(Engine::Execution::ConstraintComponent& cst, const Group& cur);
  void recurse(Scenario::ScenarioInterface&, const Group& cur);
  void recurse(Engine::Execution::TimeNodeComponent&);
  void operator()(const Engine::Execution::Context& exec_ctx);

private:
  NetworkDocumentPlugin& doc;
  const Id<Client>& self;

};
}
