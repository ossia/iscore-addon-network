#pragma once
#include <ossia/network/value/value_conversion.hpp>

#include <Netpit/Netpit.hpp>
#include <halp/audio.hpp>
#include <halp/callback.hpp>
#include <halp/controls.hpp>
#include <halp/messages.hpp>
#include <halp/meta.hpp>

#include <iostream>

namespace Netpit
{
struct AudioPit
{
public:
  halp_meta(name, "Audio Pit")
  halp_meta(category, "Network")
  halp_meta(author, "ossia team")
  halp_meta(
      description,
      "Allows to combine audio signals over the network. "
      "Every machine that runs this object instance will have its input combined with "
      "the others. "
      "On every machine, the output of the process is the resulting combination. ")
  halp_meta(c_name, "audiopit")
  halp_meta(uuid, "9307f7f3-6252-474d-8b7a-ed1ab4091e92")

  std::shared_ptr<Netpit::IAudioContext> context{};

  ~AudioPit() { unregisterSender(*this); }

  struct setup
  {
    uint64_t instance{}, subinstance{};
  };

  void prepare(setup s) { context = registerSender(s.instance, *this); }

  struct
  {
    halp::dynamic_audio_bus<"Input", float> audio;
    struct
    {
      halp__enum("Mode", Sum, List, Sum);
    } mode{};
  } inputs;

  struct
  {
    halp::dynamic_audio_bus<"Output", float> audio;
  } outputs;

  // Input of a specific client
  std::vector<AudioBuffer> current;
  void operator()(int N)
  {
    for(int c = 0; c < outputs.audio.channels; c++)
      for(int i = 0; i < N; i++)
        outputs.audio.samples[c][i] = 0.f;

    if(!context)
      return;

    // Send our current audio to the network
    context->push(tcb::span<float*>(inputs.audio.samples, inputs.audio.channels), N);

    // Read what the network has to say
    context->read(current, N);

    using mode_type = std::decay_t<decltype(inputs.mode.value)>;
    switch(inputs.mode)
    {
      case mode_type::List:
        qDebug("TODO");
        break;
      case mode_type::Sum: {
        for(int i = 0; i < N; i++)
        {
          for(auto& [channels, client] : current)
          {
            const int chans
                = std::min((int)std::ssize(channels), outputs.audio.channels);
            bool next = false;
            for(int c = 0; c < chans; ++c)
            {
              if(channels[c].empty())
              {
                next = true;
                break;
              }
              outputs.audio.samples[c][i] += channels[c].front();
              channels[c].pop_front();
            }

            if(next)
              break;
          }
        }
        break;
      }
      default:
        break;
    }
  }
};
}
