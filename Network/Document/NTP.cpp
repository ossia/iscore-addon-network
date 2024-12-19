#include <Network/Document/NTP.hpp>

#include <cmath>
#if 0
namespace Network
{
// NTP3 according to RFC 4330
struct NTP::ntp_impl
{
  char header{0x1b};

  uint8_t stratum{};
  uint8_t poll{};
  uint8_t precision{};

  uint32_t rootDelay{};
  uint32_t rootDispersion{};
  uint32_t refId{};

  uint32_t refTm_s{};
  uint32_t refTm_f{};

  uint32_t origTm_s{};
  uint32_t origTm_f{};

  uint32_t rxTm_s{};
  uint32_t rxTm_f{};

  uint32_t txTm_s{};
  uint32_t txTm_f{};
};

NTP::NTP()
    : resolver{io_service}
    , query{udp::v4(), "ntp.midway.ovh", "ntp"}
    , receiver_endpoint{*resolver.resolve(query)}
    , socket{io_service}
{
  socket.open(udp::v4());
}

NTP::~NTP() { }

NTP::duration_t NTP::get_synchronous()
{
  ntp_impl sending;
  socket.send_to(network::buffer(&sending, sizeof(ntp_impl)), receiver_endpoint);

  ntp_impl packet;
  udp::endpoint sender_endpoint;
  socket.receive_from(network::buffer(&packet, sizeof(ntp_impl)), sender_endpoint);

  packet.txTm_s = ntohl(packet.txTm_s);
  packet.txTm_f = ntohl(packet.txTm_f);

  return ntp_to_duration(packet);
}

NTP::duration_t NTP::ntp_to_duration(const NTP::ntp_impl& packet)
{
  using std::chrono::nanoseconds;
  using std::chrono::seconds;
  using namespace std;

  duration_t t{
      seconds{packet.txTm_s - ntp_delta}
      + nanoseconds{int64_t(packet.txTm_f * pow(10., 9.) / pow(2., 32.))}};

  return t;
}
}
#endif
