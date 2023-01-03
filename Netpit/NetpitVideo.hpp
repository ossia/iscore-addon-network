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
struct VideoPit
{
public:
  halp_meta(name, "Video Pit")
  halp_meta(category, "Network")
  halp_meta(author, "ossia team")
  halp_meta(
      description,
      "Allows to combine video signals over the network. "
      "Every machine that runs this object instance will have its input combined with "
      "the others. "
      "On every machine, the output of the process is the resulting combination. ")
  halp_meta(c_name, "videopit")
  halp_meta(uuid, "afbbd2b8-4e23-4b33-9736-a63b05ebf003")

  std::shared_ptr<Netpit::IVideoContext> context{};

  ~VideoPit() { unregisterSender(*this); }

  struct setup
  {
    uint64_t instance{}, subinstance{};
  };

  void prepare(setup s) { context = registerSender(s.instance, *this); }

  struct
  {
    halp::texture_input<"Input"> tex;
    struct
    {
      halp__enum("Mode", Sum, List, Sum);
    } mode{};
    halp::spinbox_i32<"Width"> width;
    halp::spinbox_i32<"Height"> height;
  } inputs;

  struct
  {
    halp::texture_output<"Output"> tex;
  } outputs;

  boost::container::vector<float> rgba_tex = halp::rgba32f_texture::allocate(320, 240);
  // Input of a specific client
  std::vector<Netpit::InboundImage> current;
  void operator()()
  {
    if(!context)
      return;

    const int w = inputs.width;
    const int h = inputs.height;

    // Send our current texture to the network
    auto& in_tex = inputs.tex.texture;
    if(in_tex.bytes && in_tex.changed)
    {
      QImage img{in_tex.bytes, in_tex.width, in_tex.height, QImage::Format_RGBA8888};
      auto scaled = img.scaled(QSize(w, h));
      halp::rgba_texture sent{.bytes = scaled.bits(), .width = w, .height = h};

      context->push(sent);
    }

    // Read what the network has to say
    context->read(current);

    auto& out_tex = outputs.tex.texture;
    int N = w * h * 4;

    outputs.tex.create(w, h);
    rgba_tex.clear();
    rgba_tex.resize(w * h * 4);

    for(auto& [tex, client] : current)
    {
      if(tex.size() != out_tex.bytesize())
      {
        qDebug() << "input texture has wrong size: " << tex.size() << out_tex.bytesize();
        continue;
      }

      unsigned char* bytes = (unsigned char*)tex.constData();
      for(int i = 0; i < N; i++)
      {
        rgba_tex[i] += bytes[i] / 255.f;
      }
    }

    for(int i = 0; i < N; i++)
    {
      out_tex.bytes[i] = std::clamp(rgba_tex[i], 0.f, 1.f) * 255.f;
    }
    out_tex.changed = true;
  }
};
}
