#include "Netpit.hpp"
#include <Netpit/NetpitMessage.hpp>
#include <Netpit/MessageContext.hpp>
#include <Network/Document/DocumentPlugin.hpp>

namespace Netpit
{

Context::~Context()
{

}

const score::DocumentContext* current{};
void setCurrentDocument(const score::DocumentContext& c)
{
  current = &c;
}

std::shared_ptr<Context> registerSender(uint64_t instance, MessagePit& p)
{
  assert(current);

  auto m = std::make_shared<MessageContext>(instance, *current);
  auto& plug = current->plugin<Network::NetworkDocumentPlugin>();
  plug.register_message_context(m);
  return m;
}

void unregisterSender(MessagePit& p)
{
  auto ctx = std::dynamic_pointer_cast<MessageContext>(p.context);
  auto& plug = ctx->ctx.plugin<Network::NetworkDocumentPlugin>();
  plug.unregister_message_context(ctx);
}


}
