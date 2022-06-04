#include "MessageContext.hpp"

#include <Network/Document/DocumentPlugin.hpp>

namespace Netpit
{

MessageContext::MessageContext(uint64_t i, const score::DocumentContext& ctx)
    : instance{i}
    , ctx{ctx}
{
}

MessageContext::~MessageContext()
{
}

void MessageContext::push(const ossia::value& val)
{
    to_network.enqueue({instance, val});
}

bool MessageContext::read(std::vector<ossia::value>& vec)
{
    Inbound b;

    bool ok = false;
    while(from_network.try_dequeue(b)) ok = true;

    if(ok)
    {
      vec = std::move(b.messages);
    }
    return ok;
}

}
