#include <Network/Document/Execution/SlavePolicy.hpp>
#include <Network/Session/MasterSession.hpp>
#include <Network/Communication/MessageMapper.hpp>
#include <iscore/model/path/PathSerialization.hpp>

namespace Network
{


SlaveExecutionPolicy::SlaveExecutionPolicy(
    Session& s,
    NetworkDocumentPlugin& doc,
    const iscore::DocumentContext& c)
{
    qDebug("SlaveExecutionPolicy");
  auto& mapi = MessagesAPI::instance();
  s.mapper().addHandler_(mapi.trigger_entered,
                         [&] (const NetworkMessage& m, Path<Scenario::TimeSyncModel> p)
  {
    auto it = doc.noncompensated.trigger_evaluation_entered.find(p);
    if(it != doc.noncompensated.trigger_evaluation_entered.end())
    {
      if(it.value())
        it.value()(m.clientId);
    }
  });

  s.mapper().addHandler_(mapi.trigger_left,
                         [&] (const NetworkMessage& m, Path<Scenario::TimeSyncModel> p)
  {
  });

  s.mapper().addHandler_(mapi.trigger_finished,
                         [&] (const NetworkMessage& m, Path<Scenario::TimeSyncModel> p, bool val)
  {
    auto it = doc.noncompensated.trigger_evaluation_finished.find(p);
    if(it != doc.noncompensated.trigger_evaluation_finished.end())
    {
      if(it.value())
        it.value()(m.clientId, val);
    }
  });

  s.mapper().addHandler_(mapi.trigger_triggered,
                         [&] (const NetworkMessage& m, Path<Scenario::TimeSyncModel> p, bool val)
  {
    auto it = doc.noncompensated.trigger_triggered.find(p);
    if(it != doc.noncompensated.trigger_triggered.end())
    {
      if(it.value())
        it.value()(m.clientId);
    }
  });
  s.mapper().addHandler_(mapi.trigger_triggered_compensated,
                         [&] (const NetworkMessage& m, Path<Scenario::TimeSyncModel> p, qint64 ns, bool val)
  {
    auto it = doc.compensated.trigger_triggered.find(p);
    if(it != doc.compensated.trigger_triggered.end())
    {
      if(it.value())
        it.value()(m.clientId, ns);
    }
  });


  s.mapper().addHandler_(mapi.interval_speed,
                         [&] (const NetworkMessage& m, Path<Scenario::IntervalModel> p, double val)
  {
    auto it = doc.noncompensated.interval_speed_changed.find(p);
    if(it != doc.noncompensated.interval_speed_changed.end())
    {
      if(it.value())
        it.value()(m.clientId, val);
    }
  });

}
}
