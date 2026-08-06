#include "Transport.hpp"

#include <Scenario/Document/BaseScenario/BaseScenario.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>

#include <score/document/DocumentContext.hpp>
#include <score/document/DocumentInterface.hpp>
#include <score/tools/Bind.hpp>

#include <core/document/Document.hpp>
#include <core/document/DocumentModel.hpp>

#include <Network/Communication/MessageMapper.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <Network/Session/Session.hpp>

namespace Network
{
namespace
{
Scenario::IntervalModel* rootInterval(const score::DocumentContext& ctx)
{
  auto* sm = score::IDocument::try_get<Scenario::ScenarioDocumentModel>(ctx.document);
  if(!sm)
    return nullptr;
  return &sm->baseScenario().interval();
}
}

void bindTransportBroadcast(
    QObject& owner, Session& session, const score::DocumentContext& ctx)
{
  auto* itv = rootInterval(ctx);
  if(!itv)
    return;

  // playPercentageChanged rather than positionChanged: the model already only
  // emits it once the score has moved by 32ms, which is about thirty messages
  // a second. positionChanged fires on every tick.
  QObject::connect(
      &itv->duration, &Scenario::IntervalDurations::playPercentageChanged, &owner,
      [&session](double pos) {
    session.broadcastToAllClients(
        session.makeMessage(MessagesAPI::instance().exec_position, pos));
      });
}

void bindTransportMirror(
    QObject& owner, Session& session, const score::DocumentContext& ctx)
{
  session.mapper().addHandler(&owner, MessagesAPI::instance().exec_position,
                              [&ctx](const NetworkMessage& m) {
    auto* itv = rootInterval(ctx);
    if(!itv)
      return;

    QDataStream s{m.data};
    double pos{};
    s >> pos;
    itv->duration.setPlayPercentage(pos);
  });
}
}
