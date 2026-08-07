#include "Transport.hpp"

#include <Scenario/Document/BaseScenario/BaseScenario.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>

#include <score/document/DocumentContext.hpp>
#include <score/document/DocumentInterface.hpp>
#include <score/model/path/ObjectPath.hpp>
#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/VisitorCommon.hpp>
#include <score/tools/Bind.hpp>

#include <core/document/Document.hpp>
#include <core/document/DocumentModel.hpp>

#include <Network/Communication/MessageMapper.hpp>
#include <Network/Communication/WireRead.hpp>
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

struct IntervalState
{
  double position{};
  bool executing{};
  bool operator==(const IntervalState&) const noexcept = default;
};

//! Every interval of the score, root included.
//!
//! findChildren rather than walking scenarios and their processes: an interval
//! can be nested under anything that holds processes, and a build without the
//! factory for one of them has a stand-in there instead -- which owns no
//! intervals, so it drops out of the list by itself.
std::vector<Scenario::IntervalModel*> allIntervals(const score::DocumentContext& ctx)
{
  auto* root = rootInterval(ctx);
  if(!root)
    return {};

  std::vector<Scenario::IntervalModel*> res{root};
  for(auto* itv : root->findChildren<Scenario::IntervalModel*>())
    res.push_back(itv);
  return res;
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
                                                     last = ossia::hash_map<
                                                         Scenario::IntervalModel*,
                                                         IntervalState>{}]() mutable {
    const auto intervals = allIntervals(ctx);
    if(intervals.empty())
    {
      last.clear();
      return;
    }

    QByteArray payload;
    QDataStream s{&payload, QIODevice::WriteOnly};
    qint32 count = 0;

    for(auto* itv : intervals)
    {
      const IntervalState now{itv->duration.playPercentage(), itv->executing()};

      // Only what moved: a score sitting at zero should not fill the socket
      // with the same numbers twenty times a second, once per interval.
      auto [it, inserted] = last.try_emplace(itv, IntervalState{});
      if(!inserted && it->second == now)
        continue;
      it->second = now;

      s << score::marshall<DataStream>(score::IDocument::path(*itv).unsafePath())
        << now.position << now.executing;
      count++;
    }

    if(count == 0)
      return;

    // Intervals that went away take their entry with them, or the map grows
    // for as long as the session lasts.
    if(last.size() > intervals.size())
    {
      ossia::hash_map<Scenario::IntervalModel*, IntervalState> alive;
      for(auto* itv : intervals)
        if(auto it = last.find(itv); it != last.end())
          alive.insert(*it);
      last = std::move(alive);
    }

    session.broadcastToAllClients(
        session.makeMessage(MessagesAPI::instance().exec_position, count, payload));
  });

  timer->start();
}

void bindTransportMirror(
    QObject& owner, Session& session, const score::DocumentContext& ctx)
{
  session.mapper().addHandler(&owner, MessagesAPI::instance().exec_position,
                              [&ctx](const NetworkMessage& m) {
    if(!readingWireData("/exec/position", [&] {
         QDataStream s{m.data};
         qint32 count{};
         QByteArray payload;
         s >> count >> payload;

         QDataStream p{payload};
         for(qint32 i = 0; i < count && !p.atEnd(); i++)
         {
           QByteArray pathBytes;
           double pos{};
           bool executing{};
           p >> pathBytes >> pos >> executing;

           const auto path = score::unmarshall<ObjectPath>(pathBytes);

           // try_find: an interval inside a process this build cannot make has
           // no counterpart here, and find() breakpoints before it throws.
           if(auto* itv = path.try_find<Scenario::IntervalModel>(ctx))
           {
             itv->duration.setPlayPercentage(pos);
             itv->setExecuting(executing);
           }
         }
       }))
      return;

    // Nothing here starts the execution timer, because nothing here executes --
    // and that timer is what asks the presenters to redraw a running interval.
    // Without it the positions arrive and no interval ever moves.
    if(auto* root = rootInterval(ctx))
    {
      auto& timer = ctx.execTimer;
      if(root->executing())
      {
        if(!timer.isActive())
          timer.start();
      }
      else if(timer.isActive())
      {
        timer.stop();
      }
    }
  });
}
}
