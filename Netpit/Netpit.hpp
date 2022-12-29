#pragma once
#include <ossia/detail/small_vector.hpp>
#include <ossia/network/value/value.hpp>

#include <boost/circular_buffer.hpp>

#include <memory>

namespace score
{
struct DocumentContext;
}
namespace Netpit
{

struct OutboundMessage
{
  // Identifier for the process instance shared across all machines
  // e.g. the object path hashed
  uint64_t instance{};
  ossia::value val;
};

struct InboundMessage
{
  ossia::value val;

  // Identifier for the client
  int32_t client{};
};

using InboundMessages = boost::container::small_vector<InboundMessage, 4>;

using AudioChannel = std::vector<float>;

struct OutboundAudio
{
  uint64_t instance{};
  std::vector<AudioChannel> channels;
};

struct InboundAudio
{
  std::vector<AudioChannel> channels;

  // Identifier for the client
  int32_t client{};
};

using InboundAudios = boost::container::small_vector<InboundAudio, 4>;

struct AudioBuffer
{
  std::vector<boost::circular_buffer<float>> channels;

  // Identifier for the client
  int32_t client{};
};

struct MessagePit;
struct AudioPit;

struct IMessageContext
{
  virtual ~IMessageContext();
  virtual void push(const ossia::value& val) = 0;
  virtual bool read(InboundMessages& vec) = 0;
};

struct IAudioContext
{
  virtual ~IAudioContext();
  virtual void push(tcb::span<float*> samples, int N) = 0;
  virtual bool read(std::vector<AudioBuffer>&, int N) = 0;
};

void setCurrentDocument(const score::DocumentContext&);
std::shared_ptr<IMessageContext> registerSender(uint64_t instance, MessagePit& p);
void unregisterSender(MessagePit& p);
std::shared_ptr<IAudioContext> registerSender(uint64_t instance, AudioPit& p);
void unregisterSender(AudioPit& p);
}

#include <verdigris>
Q_DECLARE_METATYPE(Netpit::OutboundMessage)
W_REGISTER_ARGTYPE(Netpit::OutboundMessage)
Q_DECLARE_METATYPE(Netpit::InboundMessage)
W_REGISTER_ARGTYPE(Netpit::InboundMessage)
