#pragma once
#include <Netpit/Netpit.hpp>

#include <halp/controls.hpp>
#include <halp/meta.hpp>
#include <halp/messages.hpp>
#include <halp/callback.hpp>

#include <ossia/network/value/value_conversion.hpp>

namespace Netpit
{
struct MessagePit
{
public:
  halp_meta(name, "Message Pit")
  halp_meta(c_name, "messagepit")
  halp_meta(uuid, "c97a22ee-76b0-4ced-acff-1ae8a814141f")

  std::shared_ptr<Netpit::Context> context{};

  ~MessagePit()
  {
    unregisterSender(*this);
  }

  struct setup { uint64_t instance{}, subinstance{}; };
  void prepare(setup s)
  {
    context = registerSender(s.instance, *this);
  }

  struct {
    struct { halp__enum("Mode", List, List, Average, Sum); } mode{};
    halp::toggle<"Continuous"> continuous;
  } inputs;

  struct messages {
    struct {
      halp_meta(name, "Input")
      void operator()(MessagePit& m, float v)
      {
        m.context->push(v);
      }
    } in;
  };

  struct {
    struct {
      static consteval auto name() { return "Output"; }
      halp::basic_callback<void(float)> call;
    } bang;
  } outputs;

  message_list current;
  void operator()()
  {
    // Push all our inputs to the network: done in message
    // Fetch all the network's data
    bool new_data = context->read(current);

    if(!new_data && !inputs.continuous)
      return;

    using mode_type = std::decay_t<decltype(inputs.mode.value)>;
    switch(inputs.mode)
    {
      case mode_type::List:
        // TODO
        break;
      case mode_type::Sum:
      {
        double res = 0.;
        for(auto& v : current)
          res += ossia::convert<float>(v);

        outputs.bang.call(res);
        break;
      }
      case mode_type::Average:
      {
        if(current.size() > 0)
        {
          double res = 0.;
          for(auto& v : current)
            res += ossia::convert<float>(v);

          outputs.bang.call(res / current.size());
        }
        else
        {
          outputs.bang.call(0.);
        }
        break;
      }
    }


    // Combine

  }
};
}
