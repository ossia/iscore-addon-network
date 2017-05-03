#pragma once
#include <Network/Session/ClientSession.hpp>
#include <Network/Session/ClientSessionBuilder.hpp>
#include <Network/Document/Timekeeper.hpp>

#include <Network/Document/DocumentPlugin.hpp>
// MOVEME
namespace Network
{
class ClientEditionPolicy : public EditionPolicy
{
    public:
        ClientEditionPolicy(
            ClientSession* s,
            const iscore::DocumentContext& c);

        ClientSession* session() const override
        { return m_session; }

    protected:
        void connectToOtherClient(QString ip, int port);
        ClientSession* m_session{};
        const iscore::DocumentContext& m_ctx;
        Timekeeper m_keep{*m_session};

        std::vector<std::unique_ptr<ClientSessionBuilder>> m_connections;
};

class GUIClientEditionPolicy : public ClientEditionPolicy
{
public:
  GUIClientEditionPolicy(
      ClientSession* s,
      const iscore::DocumentContext& c);

  void play() override;
  void stop() override;
};

class ISCORE_ADDON_NETWORK_EXPORT PlayerClientEditionPolicy : public ClientEditionPolicy
{
public:
  PlayerClientEditionPolicy(
      ClientSession* s,
      const iscore::DocumentContext& c);

  void play() override;
  void stop() override;
  std::function<void()> onPlay;
  std::function<void()> onStop;
};
}
