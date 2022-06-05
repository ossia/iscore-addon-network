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

bool MessageContext::read(message_list& vec)
{
    bool ok = false;
    while(from_network.try_dequeue(vec)) ok = true;
    return ok;
}

}
