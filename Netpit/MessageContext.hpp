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

  void push(std::span<float*> samples, int N) override;
  bool read(std::vector<AudioBuffer>&, int N) override;

  ossia::mpmc_queue<OutboundAudio> to_network;
  ossia::mpmc_queue<InboundAudios> from_network;
};

// Case that seems to always be needed:
// queue to update values, + operations to add / remove elements from a set
struct VideoContext : IVideoContext
{
  uint64_t instance{};
  const score::DocumentContext& ctx;

  explicit VideoContext(uint64_t i, const score::DocumentContext& ctx);

  ~VideoContext();

  void push(halp::rgba_texture samples) override;
  bool read(std::vector<InboundImage>&) override;

  ossia::mpmc_queue<OutboundImage> to_network;
  ossia::mpmc_queue<InboundImage> from_network;
};
}
