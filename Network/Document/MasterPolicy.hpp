#pragma once
#include <Network/Session/MasterSession.hpp>
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Document/Timekeeper.hpp>

namespace iscore {
struct DocumentContext;
}  // namespace iscore

namespace Network
{

class MasterEditionPolicy : public EditionPolicy
{
    public:
        MasterEditionPolicy(MasterSession* s,
                            const iscore::DocumentContext& c);

        MasterSession* session() const override
        { return m_session; }
        void play() override;
        void stop() override;

    private:
        MasterSession* m_session{};
        const iscore::DocumentContext& m_ctx;
        Timekeeper m_keep{*m_session};
};

class MasterExecutionPolicy : public ExecutionPolicy
{
public:
  MasterExecutionPolicy(
      Session& s,
      NetworkDocumentPlugin& doc,
      const iscore::DocumentContext& c);
};

class SlaveExecutionPolicy : public ExecutionPolicy
{
public:
  SlaveExecutionPolicy(
      Session& s,
      NetworkDocumentPlugin& doc,
      const iscore::DocumentContext& c);
};
}
