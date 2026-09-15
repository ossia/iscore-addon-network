#include "GroupExecution.hpp"

#include <Network/Group/Group.hpp>
#include <Network/Session/Session.hpp>

namespace Network
{
namespace
{
//! Null when the id names nobody this session knows.
const Client* findPeer(const Session& session, const Id<Client>& id) noexcept
{
  auto& local = session.localClient();
  if(local.id() == id)
    return &local;

  for(auto* c : session.remoteClients())
    if(c && c->id() == id)
      return c;

  return nullptr;
}
}

std::size_t executingClients(const Session& session, const Group& group) noexcept
{
  std::size_t n = 0;
  for(const auto& id : group.clients())
  {
    const auto* peer = findPeer(session, id);
    if(!peer || peer->role() != PeerRole::Terminal)
      ++n;
  }
  return n;
}
}
