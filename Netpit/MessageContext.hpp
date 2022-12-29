#pragma once
#include <ossia/detail/lockfree_queue.hpp>

#include <Netpit/Netpit.hpp>

namespace Netpit
{
struct MessageContext : IMessageContext
{
  uint64_t instance{};
  const score::DocumentContext& ctx;

  explicit MessageContext(uint64_t i, const score::DocumentContext& ctx);

  ~MessageContext();

  void push(const ossia::value& val) override;
  bool read(InboundMessages& vec) override;

  ossia::mpmc_queue<OutboundMessage> to_network;
  ossia::mpmc_queue<InboundMessages> from_network;
};

struct AudioContext : IAudioContext
{
  uint64_t instance{};
  const score::DocumentContext& ctx;

  explicit AudioContext(uint64_t i, const score::DocumentContext& ctx);

  ~AudioContext();

  void push(tcb::span<float*> samples, int N) override;
  bool read(std::vector<AudioBuffer>&, int N) override;

  ossia::mpmc_queue<OutboundAudio> to_network;
  ossia::mpmc_queue<InboundAudios> from_network;
};
}
