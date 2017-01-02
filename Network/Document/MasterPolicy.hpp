#pragma once
#include <Network/Session/MasterSession.hpp>

#include "DocumentPlugin.hpp"

namespace iscore {
struct DocumentContext;
}  // namespace iscore

namespace Network
{

using clk = std::chrono::high_resolution_clock;
struct ClientTimes
{
  std::chrono::nanoseconds last_sent; //! Last time a ping was sent from the master
  std::chrono::nanoseconds last_received; //! Last time a pong was received from the client.
  std::chrono::nanoseconds roundtrip_latency; //! Latency from client to master and back
  std::chrono::nanoseconds clock_difference; //! Difference between client clock and master clock
};

struct MasterTimekeep : public QObject
{
  MasterTimekeep(MasterSession& s);

  void ping_all();
  void on_pong(NetworkMessage m);

private:
  MasterSession& m_session;
  iscore::hash_map<Id<Client>, ClientTimes> m_timestamps;
  QTimer m_timer;
};

class MasterNetworkPolicy : public NetworkPolicy
{
    public:
        MasterNetworkPolicy(MasterSession* s,
                            const iscore::DocumentContext& c);

        MasterSession* session() const override
        { return m_session; }
        void play() override;

    private:
        MasterSession* m_session{};
        const iscore::DocumentContext& m_ctx;
        MasterTimekeep m_keep{*m_session};
};
}
