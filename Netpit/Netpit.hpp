#pragma once
#include <ossia/network/value/value.hpp>
#include <ossia/detail/small_vector.hpp>
#include <memory>

namespace score
{
struct DocumentContext;
}
namespace Netpit
{

using message_list = boost::container::small_vector<ossia::value, 4>;
struct Message {
  // Identifier for the process instance shared across all machines
  // e.g. the object path hashed
  uint64_t instance{};
  ossia::value val;
};

using Outbound = Message;
using Inbound = message_list;



struct MessagePit;

struct Context {
  virtual ~Context();
  virtual void push(const ossia::value& val) = 0;
  virtual bool read(message_list& vec) = 0;
};

void setCurrentDocument(const score::DocumentContext&);
std::shared_ptr<Context> registerSender(uint64_t instance, MessagePit& p);
void unregisterSender(MessagePit& p);
}


#include <verdigris>
Q_DECLARE_METATYPE(Netpit::Message)
W_REGISTER_ARGTYPE(Netpit::Message)
