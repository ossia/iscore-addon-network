#include "Transport.hpp"

#include <Scenario/Document/BaseScenario/BaseScenario.hpp>
#include <Scenario/Document/Interval/IntervalDurations.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Process/ScenarioInterface.hpp>
#include <Scenario/Process/ScenarioModel.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>

#include <score/document/DocumentContext.hpp>
#include <score/document/DocumentInterface.hpp>
#include <score/model/path/ObjectPath.hpp>
#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/VisitorCommon.hpp>
#include <score/tools/Bind.hpp>

#include <core/document/Document.hpp>
#include <core/document/DocumentModel.hpp>

#include <ossia/detail/hash_map.hpp>

#include <score/application/ApplicationContext.hpp>
#include <score/application/ApplicationComponents.hpp>
#include <core/application/ApplicationSettings.hpp>

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

  // closing() too: the model outlives its base scenario during teardown.
  if(!sm || sm->closing())
    return nullptr;

  return &sm->baseScenario().interval();
}

//! Sends where each interval has got to, as the executor moves it.
//!
//! positionChanged rather than playPercentageChanged, which is rate-limited for
//! the duration widget; scenarios are watched so intervals added later are too.
//! Coalesced to the rate score redraws at, since the executor moves an interval
//! once per audio buffer. Idle costs nothing.
class TransportBroadcaster final : public QObject
{
public:
  TransportBroadcaster(
      Session& session, const score::DocumentContext& ctx, QObject* parent)
      : QObject{parent}
      , m_session{session}
  {
    m_flush.setSingleShot(true);
    m_flush.setInterval(std::max(1, ctx.app.applicationSettings.uiEventRate));
    connect(&m_flush, &QTimer::timeout, this, &TransportBroadcaster::flush);

    if(auto* root = rootInterval(ctx))
      watchInterval(*root);
  }

private:
  void watchInterval(Scenario::IntervalModel& itv)
  {
    if(!m_watched.insert(&itv).second)
      return;

    connect(
        &itv.duration, &Scenario::IntervalDurations::positionChanged, this,
        [this, i = &itv] { markDirty(*i); });
    connect(&itv, &Scenario::IntervalModel::executingChanged, this,
            [this, i = &itv] { markDirty(*i); });
    connect(&itv, &QObject::destroyed, this, [this](QObject* o) {
      auto* i = static_cast<Scenario::IntervalModel*>(o);
      m_watched.erase(i);
      m_dirty.erase(i);
    });

    for(auto& proc : itv.processes)
      watchProcess(proc);
    itv.processes.mutable_added.connect<&TransportBroadcaster::on_processAdded>(this);
  }

  void watchProcess(Process::ProcessModel& proc)
  {
    auto* scenar = dynamic_cast<Scenario::ScenarioInterface*>(&proc);
    if(!scenar)
      return;

    for(auto& itv : scenar->getIntervals())
      watchInterval(itv);

    // Only a scenario gains and loses intervals; the others hold a fixed one.
    if(auto* model = qobject_cast<Scenario::ProcessModel*>(&proc))
      model->intervals.mutable_added
          .connect<&TransportBroadcaster::on_intervalAdded>(this);
  }

  void on_processAdded(Process::ProcessModel& proc) { watchProcess(proc); }
  void on_intervalAdded(Scenario::IntervalModel& itv) { watchInterval(itv); }

  void markDirty(Scenario::IntervalModel& itv)
  {
    m_dirty.insert(&itv);
    if(!m_flush.isActive())
      m_flush.start();
  }

  void flush()
  {
    if(m_dirty.empty())
      return;

    QByteArray payload;
    QDataStream s{&payload, QIODevice::WriteOnly};
    qint32 count = 0;

    for(auto* itv : m_dirty)
    {
      s << score::marshall<DataStream>(score::IDocument::path(*itv).unsafePath())
        << itv->duration.playPercentage() << itv->executing();
      count++;
    }
    m_dirty.clear();

    m_session.broadcastToAllClients(
        m_session.makeMessage(MessagesAPI::instance().exec_position, count, payload));
  }

  Session& m_session;
  ossia::hash_set<Scenario::IntervalModel*> m_watched;
  ossia::hash_set<Scenario::IntervalModel*> m_dirty;
  QTimer m_flush;
};
}

void bindTransportBroadcast(
    QObject& owner, Session& session, const score::DocumentContext& ctx)
{
  new TransportBroadcaster{session, ctx, &owner};
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

    // Nothing here executes, so nothing else starts the timer that repaints
    // a running interval.
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
