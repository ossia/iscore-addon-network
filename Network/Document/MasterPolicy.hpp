#pragma once
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Document/Timekeeper.hpp>
#include <Network/Session/MasterSession.hpp>

namespace Network
{
class MasterEditionPolicy : public EditionPolicy
{
public:
  MasterEditionPolicy(MasterSession* s, const score::DocumentContext& c);

  MasterSession* session() const override { return m_session; }
  void play() override;
  void stop() override;

  Timekeeper& timekeeper() { return m_keep; }

private:
  MasterSession* m_session{};
  const score::DocumentContext& m_ctx;
  Timekeeper m_keep{*m_session};
};
}
