#pragma once
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Document/Timekeeper.hpp>
#include <Network/Session/ClientSession.hpp>
#include <Network/Session/ClientSessionBuilder.hpp>
// MOVEME
namespace Network
{
class ClientEditionPolicy : public EditionPolicy
{
public:
  ClientEditionPolicy(ClientSession* s, const score::DocumentContext& c);

  ClientSession* session() const override { return m_session; }

protected:
  void connectToOtherClient(QString ip, int port);
  ClientSession* m_session{};
  const score::DocumentContext& m_ctx;
  Timekeeper m_keep{*m_session};

  std::vector<std::unique_ptr<ClientSessionBuilder>> m_connections;
};

class GUIClientEditionPolicy : public ClientEditionPolicy
{
public:
  GUIClientEditionPolicy(ClientSession* s, const score::DocumentContext& c);

  void play() override;
  void stop() override;
};

/**
 * @brief A client that edits the score but never runs it.
 *
 * Everything that makes remote edition work is inherited unchanged: commands,
 * undo, locks and the rpc channel all replicate exactly as for any peer. Only
 * the meaning of transport differs. Play here is a request addressed to the
 * host, and the host's own /play is not an instruction to start anything --
 * there is nothing on this machine to start.
 */
class SCORE_ADDON_NETWORK_EXPORT TerminalEditionPolicy : public ClientEditionPolicy
{
public:
  TerminalEditionPolicy(ClientSession* s, const score::DocumentContext& c);

  void play() override;
  void stop() override;

private:
  void requestPlay();
  void requestStop();
};

class SCORE_ADDON_NETWORK_EXPORT PlayerClientEditionPolicy : public ClientEditionPolicy
{
public:
  PlayerClientEditionPolicy(ClientSession* s, const score::DocumentContext& c);

  void play() override;
  void stop() override;
  std::function<void()> onPlay;
  std::function<void()> onStop;
};
}
