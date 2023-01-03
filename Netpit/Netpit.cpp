#include "Netpit.hpp"

#include <Netpit/MessageContext.hpp>
#include <Netpit/NetpitAudio.hpp>
#include <Netpit/NetpitMessage.hpp>
#include <Netpit/NetpitVideo.hpp>
#include <Network/Document/DocumentPlugin.hpp>

namespace Netpit
{

IMessageContext::~IMessageContext() { }
IAudioContext::~IAudioContext() { }
IVideoContext::~IVideoContext() { }

const score::DocumentContext* current{};
void setCurrentDocument(const score::DocumentContext& c)
{
  current = &c;
}

std::shared_ptr<IMessageContext> registerSender(uint64_t instance, MessagePit& p)
{
  assert(current);

  if(auto plug = current->findPlugin<Network::NetworkDocumentPlugin>())
  {
    auto m = std::make_shared<MessageContext>(instance, *current);
    plug->register_message_context(m);
    return m;
  }
  else
  {
    return {};
  }
}

void unregisterSender(MessagePit& p)
{
  if(p.context)
  {
    auto ctx = std::dynamic_pointer_cast<MessageContext>(p.context);
    auto& plug = ctx->ctx.plugin<Network::NetworkDocumentPlugin>();
    plug.unregister_message_context(ctx);
  }
}

std::shared_ptr<IAudioContext> registerSender(uint64_t instance, AudioPit& p)
{
  assert(current);

  if(auto plug = current->findPlugin<Network::NetworkDocumentPlugin>())
  {
    auto m = std::make_shared<AudioContext>(instance, *current);
    plug->register_audio_context(m);
    return m;
  }
  else
  {
    return {};
  }
}

void unregisterSender(AudioPit& p)
{
  if(p.context)
  {
    auto ctx = std::dynamic_pointer_cast<AudioContext>(p.context);
    auto& plug = ctx->ctx.plugin<Network::NetworkDocumentPlugin>();
    plug.unregister_audio_context(ctx);
  }
}

std::shared_ptr<IVideoContext> registerSender(uint64_t instance, VideoPit& p)
{
  assert(current);

  if(auto plug = current->findPlugin<Network::NetworkDocumentPlugin>())
  {
    auto m = std::make_shared<VideoContext>(instance, *current);
    plug->register_video_context(m);
    return m;
  }
  else
  {
    return {};
  }
}

void unregisterSender(VideoPit& p)
{
  if(p.context)
  {
    auto ctx = std::dynamic_pointer_cast<VideoContext>(p.context);
    auto& plug = ctx->ctx.plugin<Network::NetworkDocumentPlugin>();
    plug.unregister_video_context(ctx);
  }
}

}
