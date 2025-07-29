#include "MessageContext.hpp"

#include <Network/Document/DocumentPlugin.hpp>

namespace Netpit
{

MessageContext::MessageContext(uint64_t i, const score::DocumentContext& ctx)
    : instance{i}
    , ctx{ctx}
{
}

MessageContext::~MessageContext() { }

void MessageContext::push(const ossia::value& val)
{
  to_network.enqueue({instance, val});
}

bool MessageContext::read(InboundMessages& vec)
{
  bool ok = false;
  while(from_network.try_dequeue(vec))
    ok = true;
  return ok;
}

AudioContext::AudioContext(uint64_t i, const score::DocumentContext& ctx)
    : instance{i}
    , ctx{ctx}
{
}

AudioContext::~AudioContext() { }

void AudioContext::push(std::span<float*> data, int N)
{
  const int channels = data.size();
  const auto samples = data.data();

  std::vector<std::vector<float>> vec;
  vec.reserve(channels);
  for(int i = 0; i < channels; i++)
  {
    vec.emplace_back(samples[i], samples[i] + N);
  }
  to_network.enqueue({instance, std::move(vec)});
}

bool AudioContext::read(std::vector<AudioBuffer>& data, int N)
{
  // FIXME we have to write the audio data in a queue
  // FIXME potential modes: sum channels or append them
  InboundAudios vec;

  while(from_network.try_dequeue(vec))
  {
    for(auto& ins : vec)
    {
      auto it = std::find_if(data.begin(), data.end(), [&](const AudioBuffer& d) {
        return d.client == ins.client;
      });

      if(it != data.end())
      {
        int n_circbuf = it->channels.size();
        int n_channels_net = ins.channels.size();

        // Append to the circular buffers for each channel
        for(int i = 0, n = std::min(n_circbuf, n_channels_net); i < n; i++)
        {
          auto& input = ins.channels[i];
          auto& circbuf = it->channels[i];
          circbuf.insert(circbuf.end(), input.begin(), input.end());
        }

        // Add new channels if e.g. we currently have 0 channels and remote have 2
        for(int i = n_circbuf; i < n_channels_net; i++)
        {
          auto& input = ins.channels[i];
          boost::circular_buffer<float> buf;
          buf.resize(4096); // our initial safety buffer
          buf.insert(buf.end(), input.begin(), input.end());
          it->channels.emplace_back(std::move(buf));
        }
      }
      else
      {
        // Register a new client
        AudioBuffer b;
        b.client = ins.client;
        for(auto& input : ins.channels)
        {
          boost::circular_buffer<float> buf;
          buf.resize(4096); // our initial safety buffer
          buf.insert(buf.end(), input.begin(), input.end());
          b.channels.emplace_back(std::move(buf));
        }
        data.push_back(b);
      }
    }
  }
  return true;
}

VideoContext::VideoContext(uint64_t i, const score::DocumentContext& ctx)
    : instance{i}
    , ctx{ctx}
{
}

VideoContext::~VideoContext() { }

void VideoContext::push(halp::rgba_texture data)
{
  to_network.enqueue(
      {instance,
       QByteArray(reinterpret_cast<const char*>(data.bytes), data.bytesize())});
}

bool VideoContext::read(std::vector<InboundImage>& data)
{
  InboundImage ins;
  while(from_network.try_dequeue(ins))
  {
    auto it = std::find_if(data.begin(), data.end(), [&](const InboundImage& d) {
      return d.client == ins.client;
    });

    if(it != data.end())
    {
      it->texture = ins.texture;
    }
    else
    {
      data.push_back(ins);
    }
  }

  return true;
}
}
