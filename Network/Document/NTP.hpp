#pragma once
#include <ossia/detail/config.hpp>

#include <boost/asio.hpp>

#include <chrono>
#include <thread>

namespace Network
{
namespace network = boost::asio;
using udp = network::ip::udp;

class NTP
{
  static const constexpr auto ntp_delta = 2208988800ull;
  using duration_t
      = std::chrono::duration<int64_t, std::ratio<1l, 1000000000l>>;

public:
  NTP();
  ~NTP();

  duration_t get_synchronous();

private:
  struct ntp_impl;
  static duration_t ntp_to_duration(const ntp_impl& packet);

  network::io_service io_service;
  udp::resolver resolver;
  udp::resolver::query query;
  udp::endpoint receiver_endpoint;
  udp::socket socket;
};
}
