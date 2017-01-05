#pragma once
#include <QObject>
#include <iscore/tools/Todo.hpp>
#include <iscore/tools/std/HashMap.hpp>
#include <Network/Session/Session.hpp>
#include <chrono>
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


struct Timekeeper final : public QObject
{
  Timekeeper(Session& s):
    m_session{s}
  {
    startTimer(1000, Qt::PreciseTimer);

    for(auto client : m_session.remoteClients())
    {
      m_timestamps.insert({client->id(), ClientTimes{}});
    }

    con(m_session, &Session::clientAdded,
        this, [=] (auto client) {
      m_timestamps.insert({client->id(), ClientTimes{}});
    });
    // TODO clientRemoved
  }

  void ping_all()
  {
    auto t = clk::now().time_since_epoch();
    m_session.broadcastToAllClients(m_session.makeMessage("/ping"));

    auto b = m_timestamps.begin();
    auto e = m_timestamps.end();
    for(auto it = b; it != e; ++it)
    {
      it.value().last_sent = t;
    }
  }
  void on_pong(NetworkMessage m)
  {
    auto pong_date = clk::now().time_since_epoch();

    QDataStream reader(m.data);

    qint64 ns;
    reader >> ns;

    auto it = m_timestamps.find(m.clientId);
    if(it != m_timestamps.end())
    {
      ClientTimes& times = it.value();
      times.last_received = pong_date;
      times.roundtrip_latency = times.last_received - times.last_sent;

      // Note : here we just assume that the half-trip latency is half the round trip latency.
      times.clock_difference = std::chrono::nanoseconds(ns) - (times.last_sent + times.roundtrip_latency / 2);
    }
  }

  static auto us(const std::chrono::nanoseconds u)
  { return std::chrono::duration_cast<std::chrono::microseconds>(u).count(); }

  void debug()
  {
    for(auto clt : m_timestamps)
    {
      qDebug() << clt.first << us(clt.second.roundtrip_latency) <<  us(clt.second.clock_difference);
    }
  }

  void timerEvent(QTimerEvent *event) override
  {
    ping_all();
    debug();
  }

private:
  Session& m_session;
  iscore::hash_map<Id<Client>, ClientTimes> m_timestamps;
};

}
