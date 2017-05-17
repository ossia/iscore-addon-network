#include "SharedScenarioPolicy.hpp"
#include <Network/Document/Execution/SharedNonCompensatedExpressions.hpp>
#include <Network/Document/Execution/SharedCompensatedExpressions.hpp>

namespace Network
{

void SharedScenarioPolicy::operator()(
    Engine::Execution::ProcessComponent& c,
    Scenario::ScenarioInterface& ip,
    const Group& cur)
{
  for(Scenario::TimeNodeModel& tn : ip.getTimeNodes())
  {
    auto comp = iscore::findComponent<Engine::Execution::TimeNodeComponent>(tn.components());
    if(comp)
    {
      operator()(*comp, cur);
    }
  }

  for(Scenario::EventModel& tn : ip.getTimeNodes())
  {
    auto comp = iscore::findComponent<Engine::Execution::TimeNodeComponent>(tn.components());
    if(comp)
    {
      operator()(*comp, cur);
    }
  }

  for(Scenario::ConstraintModel& cst : ip.getConstraints())
  {
    auto comp = iscore::findComponent<Engine::Execution::ConstraintComponent>(cst.components());
    if(comp)
      operator()(*comp, cur);
  }

}

void SharedScenarioPolicy::operator()(
    Engine::Execution::ConstraintComponent& cst,
    const Group& cur)
{
  const auto& str = Constants::instance();

  Scenario::ConstraintModel& constraint = cst.iscoreConstraint();

  const Group& cur_group = getGroup(ctx.gm, cur, constraint);

  // Execution speed
  {
    auto& session = ctx.session;
    auto& mapi = ctx.mapi;
    auto master = ctx.master;
    Path<Scenario::ConstraintModel> path{cst.iscoreConstraint()};
    // This -> Master
    auto block = std::make_shared<bool>(false);
    QObject::connect(&constraint.duration, &Scenario::ConstraintDurations::executionSpeedChanged,
                     &cst, [=,&session,&mapi] (double s) {
      // TODO handle sync / async. Even though sync does not really make sense here...
      if(!(*block))
        session.sendMessage(master, session.makeMessage(mapi.constraint_speed, path, s));
    });

    // Master -> This
    ctx.doc.noncompensated.constraint_speed_changed.emplace(path, [&,block] (const Id<Client>& orig, double s) {
      *block = true;
      constraint.duration.setExecutionSpeed(s);
      *block = false;
    });

  }

  // Muting
  {
    const bool isMuted = !cur_group.hasClient(ctx.self);
    // Mute the processes that are not meant to execute there.
    constraint.setExecutionState(isMuted
                                 ? Scenario::ConstraintExecutionState::Muted
                                 : Scenario::ConstraintExecutionState::Enabled);

    for(const auto& process : cst.processes())
    {
      auto& proc = process.second->OSSIAProcess();
      proc.mute(isMuted);
    }
  }

  // Recursion
  {
    auto constraint_sharemode = get_metadata<QString>(constraint, str.sharemode);
    if(!constraint_sharemode || constraint_sharemode->isEmpty())
      constraint_sharemode = str.shared;

    for(const auto& process : cst.processes())
    {
      auto ip = dynamic_cast<Scenario::ScenarioInterface*>(&process.second->process());
      if(ip)
      {
        auto sharemode = get_metadata<QString>(process.second->process(), str.sharemode);
        if(!sharemode || sharemode->isEmpty())
          sharemode = constraint_sharemode;

        if(*sharemode == str.shared)
        {
          SharedScenarioPolicy{ctx}(*process.second, *ip, cur_group);
        }
        else if(*sharemode == str.mixed)
        {
          MixedScenarioPolicy{ctx}(*process.second, *ip, cur_group);
        }
        else if(*sharemode == str.free)
        {
          FreeScenarioPolicy{ctx}(*process.second, *ip, cur_group);
        }
      }
    }
  }
}

void SharedScenarioPolicy::operator()(Engine::Execution::EventComponent& cst, const Group& cur)
{

}


void SharedScenarioPolicy::operator()(
    Engine::Execution::TimeNodeComponent& comp,
    const Group& parent_group)
{
  auto& mapi = ctx.mapi;
  // First fetch the required variables.
  const Group& tn_group = getGroup(ctx.gm, parent_group, comp.iscoreTimeNode());

  auto sync = getInfos(comp.iscoreTimeNode());
  Path<Scenario::TimeNodeModel> path{comp.iscoreTimeNode()};

  if(comp.iscoreTimeNode().trigger()->active())
  {
    auto& session = ctx.session;
    auto master = ctx.master;
    // Each trigger sends its own data, the master will choose the relevant info
    comp.OSSIATimeNode()->entered_evaluation.add_callback([=,&mapi,&session] {
        qDebug("SharedScenarioPolicy: trigger entered");
      session.emitMessage(master, session.makeMessage(mapi.trigger_entered, path));
    });
    comp.OSSIATimeNode()->left_evaluation.add_callback([=,&mapi,&session] {
        qDebug("SharedScenarioPolicy: trigger left");
      session.emitMessage(master, session.makeMessage(mapi.trigger_left, path));
    });
    comp.OSSIATimeNode()->finished_evaluation.add_callback([=,&mapi,&session] (bool b) {
        qDebug("SharedScenarioPolicy: trigger finished");
      // b : max bound reached
      session.emitMessage(master, session.makeMessage(mapi.trigger_finished, path, b));
    });
    comp.OSSIATimeNode()->triggered.add_callback([=,&mapi,&session] {
        qDebug("SharedScenarioPolicy: trigger triggered");
      session.emitMessage(master, session.makeMessage(mapi.trigger_triggered, path));
    });

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
    Engine::Execution::TimeNodeComponent& comp,
    Path<Scenario::TimeNodeModel> p,
    const Group& tn_group,
    SyncMode sync)
{
  // Handle the "talk to master" case
  if(ctx.master == ctx.self)
  {
    NetworkExpressionData exp{comp};
    exp.thisGroup = tn_group.id();
    exp.sync = sync;
    exp.pol = ExpressionPolicy::OnFirst; // TODO another

    auto scenar = dynamic_cast<Scenario::ScenarioInterface*>(comp.iscoreTimeNode().parent());

    auto constraint_group = [&] (const Id<Scenario::ConstraintModel>& cst_id)
    {
      auto& cst = scenar->constraint(cst_id);
      auto& grp = getGroup(ctx.gm, tn_group, cst);
      return grp.id();
    };

    {
      // Find all the previous ConstraintComponents.
      auto csts = Scenario::previousConstraints(comp.iscoreTimeNode(), *scenar);
      exp.prevGroups.reserve(csts.size());
      ossia::transform(csts, std::back_inserter(exp.prevGroups), constraint_group);
    }

    {
      auto csts = Scenario::nextConstraints(comp.iscoreTimeNode(), *scenar);
      exp.nextGroups.reserve(csts.size());
      ossia::transform(csts, std::back_inserter(exp.nextGroups), constraint_group);
    }

    ctx.doc.noncompensated.network_expressions.emplace(std::move(p), std::move(exp));
  }

}
}
