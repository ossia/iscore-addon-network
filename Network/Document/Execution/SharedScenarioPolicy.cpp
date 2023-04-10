#include "SharedScenarioPolicy.hpp"

#include <Scenario/Document/Event/EventExecution.hpp>

#include <score/model/ComponentUtils.hpp>
#include <score/tools/Bind.hpp>

#include <Network/Document/Execution/SharedCompensatedExpressions.hpp>
#include <Network/Document/Execution/SharedNonCompensatedExpressions.hpp>

namespace Network
{

void SharedScenarioPolicy::operator()(
    Execution::ProcessComponent& c, Scenario::ScenarioInterface& ip, const Group& cur)
{
  for(Scenario::TimeSyncModel& tn : ip.getTimeSyncs())
  {
    auto comp = score::findComponent<Execution::TimeSyncComponent>(tn.components());
    if(comp)
    {
      operator()(*comp, cur);
    }
  }

  for(Scenario::EventModel& tn : ip.getEvents())
  {
    auto comp = score::findComponent<Execution::EventComponent>(tn.components());
    if(comp)
    {
      operator()(*comp, cur);
    }
  }

  for(Scenario::IntervalModel& cst : ip.getIntervals())
  {
    auto comp = score::findComponent<Execution::IntervalComponent>(cst.components());
    if(comp)
      operator()(*comp, cur);
  }
}

void SharedScenarioPolicy::operator()(
    Execution::IntervalComponent& cst, const Group& cur)
{
  const auto& str = Constants::instance();

  Scenario::IntervalModel& interval = cst.scoreInterval();

  const Group& cur_group = getGroup(ctx.doc, ctx.gm, cur, interval);

  qDebug() << "Interval: " << interval.metadata().getName() << ":" << cur_group.name()
           << cur_group.hasClient(ctx.self);
  // Execution speed
  {
    auto& session = ctx.session;
    auto& mapi = ctx.mapi;
    auto master = ctx.master;
    Path<Scenario::IntervalModel> path{cst.scoreInterval()};
    // This -> Master
    auto block = std::make_shared<bool>(false);
    QObject::connect(
        &interval.duration, &Scenario::IntervalDurations::speedChanged, &cst,
        [=, &session, &mapi](double s) {
      // TODO handle sync / async. Even though sync does not really make
      // sense here...
      if(!(*block))
        session.sendMessage(master, session.makeMessage(mapi.interval_speed, path, s));
        });

    // Master -> This
    ctx.doc.noncompensated.interval_speed_changed.emplace(
        path, [&, block](const Id<Client>& orig, double s) {
          *block = true;
          interval.duration.setSpeed(s);
          *block = false;
        });
  }

  // Muting
  {
    const bool isMuted = !cur_group.hasClient(ctx.self);
    // Mute the processes that are not meant to execute there.
    interval.setExecutionState(
        isMuted ? Scenario::IntervalExecutionState::Muted
                : Scenario::IntervalExecutionState::Enabled);

    for(const auto& process : cst.processes())
    {
      auto& proc = process.second->OSSIAProcess();
      proc.mute(isMuted);
    }
  }

  // Recursion
  {
    auto interval_meta = ctx.doc.get_metadata(interval);
    auto interval_sharemode
        = interval_meta.sharemode ? *interval_meta.sharemode : ShareMode::Shared;

    for(const auto& process : cst.processes())
    {
      auto ip = dynamic_cast<Scenario::ScenarioInterface*>(&process.second->process());
      if(ip)
      {
        auto process_meta = ctx.doc.get_metadata(process.second->process());
        auto sharemode
            = process_meta.sharemode ? *process_meta.sharemode : interval_sharemode;

        switch(sharemode)
        {
          case ShareMode::Shared:
            SharedScenarioPolicy{ctx}(*process.second, *ip, cur_group);
            break;
          case ShareMode::Mixed:
            MixedScenarioPolicy{ctx}(*process.second, *ip, cur_group);
            break;
          case ShareMode::Free:
            FreeScenarioPolicy{ctx}(*process.second, *ip, cur_group);
            break;
        }
      }
      else
      {
        // FIXME: disable controls for processes in "free" mode
        process.second->OSSIAProcess().enable(cur_group.hasClient(ctx.self));
      }
    }
  }
}

void SharedScenarioPolicy::operator()(Execution::EventComponent& cst, const Group& cur)
{
}

struct SharedTimesyncCallbacks : public ossia::time_sync_callback
{
public:
  SharedTimesyncCallbacks(
      Id<Client> m, const Path<Scenario::TimeSyncModel>& p, const MessagesAPI& ma,
      Session& s)
      : master{m}
      , path{p}
      , mapi{ma}
      , session{s}
  {
  }

  ~SharedTimesyncCallbacks() { }

  Id<Client> master;
  Path<Scenario::TimeSyncModel> path;
  const MessagesAPI& mapi;
  Session& session;

  void entered_evaluation() override
  {
    qDebug("SharedScenarioPolicy: trigger entered");
    session.emitMessage(master, session.makeMessage(mapi.trigger_entered, path));
  }
  void left_evaluation() override
  {
    qDebug("SharedScenarioPolicy: trigger left");
    session.emitMessage(master, session.makeMessage(mapi.trigger_left, path));
  }

  void triggered() override
  {
    qDebug("SharedScenarioPolicy: trigger triggered");
    session.emitMessage(master, session.makeMessage(mapi.trigger_triggered, path));
  }
};

void SharedScenarioPolicy::operator()(
    Execution::TimeSyncComponent& comp, const Group& parent_group)
{
  auto& mapi = ctx.mapi;
  // First fetch the required variables.
  const Group& tn_group = getGroup(ctx.doc, ctx.gm, parent_group, comp.scoreTimeSync());

  auto sync = getInfos(ctx.doc, comp.scoreTimeSync());
  Path<Scenario::TimeSyncModel> path{comp.scoreTimeSync()};

  if(comp.scoreTimeSync().active())
  {
    auto& session = ctx.session;
    auto master = ctx.master;
    // Each trigger sends its own data, the master will choose the relevant
    // info
    comp.OSSIATimeSync()->callbacks.callbacks.push_back(
        new SharedTimesyncCallbacks{master, path, mapi, session});

    // If this group has this expression
    // Since we're in the SharedPolicy, everybody will get the same information
    if(tn_group.hasClient(ctx.self))
    {
      // We will actually evaluate the expression.

      switch(sync)
      {
        case SyncMode::NonCompensatedSync:
          SharedNonCompensatedSyncInGroup{}(ctx, comp, path);
          break;
        case SyncMode::NonCompensatedAsync:
          SharedNonCompensatedAsyncInGroup{}(ctx, comp, path);
          break;
        case SyncMode::CompensatedSync:
          SharedCompensatedSyncInGroup{}(ctx, comp, path);
          break;
        case SyncMode::CompensatedAsync:
          SharedCompensatedAsyncInGroup{}(ctx, comp, path);
          break;
      }
    }
    else
    {
      // Not in the group : we wait.
      switch(sync)
      {
        case SyncMode::NonCompensatedSync:
          SharedNonCompensatedSyncOutOfGroup{}(ctx, comp, path);
          break;
        case SyncMode::NonCompensatedAsync:
          SharedNonCompensatedAsyncOutOfGroup{}(ctx, comp, path);
          break;
        case SyncMode::CompensatedSync:
          SharedCompensatedSyncOutOfGroup{}(ctx, comp, path);
          break;
        case SyncMode::CompensatedAsync:
          SharedCompensatedAsyncOutOfGroup{}(ctx, comp, path);
          break;
      }
    }

    setupMaster(comp, path, tn_group, sync);
  }
  else
  {
    // Trigger not active. Maybe we could explicitely resynchronize here...
  }
}

void SharedScenarioPolicy::setupMaster(
    Execution::TimeSyncComponent& comp, Path<Scenario::TimeSyncModel> p,
    const Group& tn_group, SyncMode sync)
{
  // Handle the "talk to master" case
  if(ctx.master == ctx.self)
  {
    NetworkExpressionData exp{comp};
    exp.thisGroup = tn_group.id();
    exp.sync = sync;
    exp.pol = ExpressionPolicy::OnFirst; // TODO another

    auto scenar
        = dynamic_cast<Scenario::ScenarioInterface*>(comp.scoreTimeSync().parent());

    auto interval_group = [&](const Id<Scenario::IntervalModel>& cst_id) {
      auto& cst = scenar->interval(cst_id);
      auto& grp = getGroup(ctx.doc, ctx.gm, tn_group, cst);
      return grp.id();
    };

    {
      // Find all the previous IntervalComponents.
      auto csts = Scenario::previousIntervals(comp.scoreTimeSync(), *scenar);
      exp.prevGroups.reserve(csts.size());
      ossia::transform(csts, std::back_inserter(exp.prevGroups), interval_group);
    }

    {
      auto csts = Scenario::nextIntervals(comp.scoreTimeSync(), *scenar);
      exp.nextGroups.reserve(csts.size());
      ossia::transform(csts, std::back_inserter(exp.nextGroups), interval_group);
    }

    ctx.doc.noncompensated.network_expressions.emplace(std::move(p), std::move(exp));
  }
}
}
