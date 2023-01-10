#pragma once
#include <ossia/network/value/value_conversion.hpp>

#include <QDebug>
#include <QImage>

#include <Netpit/Netpit.hpp>
#include <halp/audio.hpp>
#include <halp/callback.hpp>
#include <halp/controls.hpp>
#include <halp/messages.hpp>
#include <halp/meta.hpp>

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
    halp::spinbox_i32<"Width", halp::range{1, 2048, 1280}> width;
    halp::spinbox_i32<"Height", halp::range{1, 2048, 720}> height;
  } inputs;

  struct
  {
    halp::texture_output<"Output"> tex;
  } outputs;

  boost::container::vector<float> rgba_tex = halp::rgba32f_texture::allocate(1280, 720);
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
      // FIXME instead have a buffer and just do sws_rescale...
      QImage img{in_tex.bytes, in_tex.width, in_tex.height, QImage::Format_RGBA8888};
      auto scaled = img.scaled(QSize(w, h));
      halp::rgba_texture sent{.bytes = scaled.bits(), .width = w, .height = h};

      context->push(sent);
    }

    // Read what the network has to say
    context->read(current);

    if(current.empty())
      return;

    auto& out_tex = outputs.tex.texture;
    const int N = w * h * 4;

    outputs.tex.create(w, h);
    boost::container::small_vector<unsigned char*, 16> textures_to_copy;
    for(auto& [tex, client] : current)
    {
      if(tex.size() != out_tex.bytesize())
      {
        qDebug() << "input texture has wrong size: " << tex.size() << out_tex.bytesize();
        continue;
      }
      textures_to_copy.push_back((unsigned char*)tex.data());
    }

    if(textures_to_copy.empty())
      return;

    const std::size_t T = textures_to_copy.size();

    auto copy_tex = [&](unsigned char* bytes) {
      for(int i = 0; i < N; i++)
      {
        out_tex.bytes[i] = bytes[i];
      }
    };
    auto add_tex = [&](auto... bytes) {
      for(int i = 0; i < N; i++)
      {
        out_tex.bytes[i] += std::clamp((int(bytes[i]) + ...), 0, 255);
      }
    };

    switch(T)
    {
      case 0:
        return;
      case 1:
        copy_tex(textures_to_copy[0]);
        break;
      case 2:
        add_tex(textures_to_copy[0], textures_to_copy[1]);
        break;
      case 3:
        add_tex(textures_to_copy[0], textures_to_copy[1], textures_to_copy[2]);
        break;
      case 4:
        add_tex(
            textures_to_copy[0], textures_to_copy[1], textures_to_copy[2],
            textures_to_copy[3]);
        break;
      default: {
        rgba_tex.clear();
        rgba_tex.assign(textures_to_copy[0], textures_to_copy[0] + N);

        for(std::size_t tex = 1; tex < T; tex++)
        {
          unsigned char* bytes = textures_to_copy[tex];
          for(int i = 0; i < N; i++)
          {
            rgba_tex[i] += bytes[i];
          }
        }

        for(int i = 0; i < N; i++)
        {
          out_tex.bytes[i] = std::clamp(rgba_tex[i], 0.f, 255.f);
        }
      }
    }

    out_tex.changed = true;
  }
};
}
