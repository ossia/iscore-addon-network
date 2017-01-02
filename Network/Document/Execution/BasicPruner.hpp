#pragma once
#include <iscore/model/Identifier.hpp>
namespace Engine
{
namespace Execution
{
class ConstraintComponent;
class Context;
}
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
  void operator()(const Engine::Execution::Context& exec_ctx);

private:
  NetworkDocumentPlugin& doc;
  const Id<Client>& self;

};
}
