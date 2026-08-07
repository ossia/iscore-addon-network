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

#include <QTimer>

namespace Network
{
namespace
{
Scenario::IntervalModel* rootInterval(const score::DocumentContext& ctx)
{
  auto* sm = score::IDocument::try_get<Scenario::ScenarioDocumentModel>(ctx.document);

  // closing() as well as non-null: the model outlives its base scenario while
  // a document is torn down, and anything polling it -- a timer, the timing
  // widget -- keeps running until it is gone.
  if(!sm || sm->closing())
    return nullptr;

  return &sm->baseScenario().interval();
}
}

void bindTransportBroadcast(
    QObject& owner, Session& session, const score::DocumentContext& ctx)
{
  // Polled rather than driven by playPercentageChanged. That signal fires only
  // when a *single* update moves the score by more than 32ms, and an executor
  // tick is an audio buffer -- about ten. So it almost never fires, which read
  // as the remote clock advancing a little and then stopping. score's own time
  // display does not use it either: it reads playPercentage on a timer, which
  // is what this now does.
  auto* timer = new QTimer{&owner};
  timer->setInterval(1000 / 20);

  QObject::connect(timer, &QTimer::timeout, &owner, [&session, &ctx,
                                                      last = -1.]() mutable {
    auto* itv = rootInterval(ctx);
    if(!itv)
      return;

    const double pos = itv->duration.playPercentage();

    // Only when it moves: a stopped score should not fill the socket with the
    // same number twenty times a second. Per connection, not per process --
    // one score's position says nothing about another's.
    if(pos == last)
      return;
    last = pos;

    session.broadcastToAllClients(
        session.makeMessage(MessagesAPI::instance().exec_position, pos));
  });

  timer->start();
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
