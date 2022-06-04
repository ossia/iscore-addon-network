#pragma once
#include <Netpit/Netpit.hpp>
#include <ossia/detail/lockfree_queue.hpp>

namespace Netpit
{
struct MessageContext : Context
{
  uint64_t instance{};
  const score::DocumentContext& ctx;

  explicit MessageContext(uint64_t i, const score::DocumentContext& ctx);

  ~MessageContext();

  void push(const ossia::value& val);
  bool read(std::vector<ossia::value>& vec);

  ossia::mpmc_queue<Outbound> to_network;
  ossia::mpmc_queue<Inbound> from_network;
};
}
