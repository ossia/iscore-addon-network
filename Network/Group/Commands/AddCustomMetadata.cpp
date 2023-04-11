
#include <Scenario/Document/Event/EventModel.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/TimeSync/TimeSyncModel.hpp>
#include <Scenario/Process/Algorithms/Accessors.hpp>
#include <Scenario/Process/ScenarioModel.hpp>

#include <score/command/Dispatchers/CommandDispatcher.hpp>
#include <score/document/DocumentContext.hpp>
#include <score/model/path/PathSerialization.hpp>
#include <score/selection/SelectionStack.hpp>

#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Group/Commands/AddCustomMetadata.hpp>

template <typename T, typename V>
struct TSerializer<DataStream, Network::Command::MetadataUndoRedo<T, V>>
{
  static void readFrom(
      DataStream::Serializer& s, const Network::Command::MetadataUndoRedo<T, V>& obj)
  {
    s.stream() << obj.path << obj.before;
  }

  static void
  writeTo(DataStream::Deserializer& s, Network::Command::MetadataUndoRedo<T, V>& obj)
  {
    s.stream() >> obj.path >> obj.before;
  }
};
namespace Network::Command
{
template <typename MetadataType>
void setup_init_metadata(auto& command, const Selection& s, auto read)
{

  auto init_itv = [&](Scenario::IntervalModel* elt) {
    MetadataUndoRedo<Scenario::IntervalModel, MetadataType> m;
    m.path = score::IDocument::path(*elt);
    m.before = read(*elt);
    command.metadatas.intervals.push_back(std::move(m));
  };
  auto init_ev = [&](Scenario::EventModel* elt) {
    MetadataUndoRedo<Scenario::EventModel, MetadataType> m;
    m.path = score::IDocument::path(*elt);
    m.before = read(*elt);
    command.metadatas.events.push_back(std::move(m));
  };
  auto init_s = [&](Scenario::TimeSyncModel* elt) {
    MetadataUndoRedo<Scenario::TimeSyncModel, MetadataType> m;
    m.path = score::IDocument::path(*elt);
    m.before = read(*elt);
    command.metadatas.nodes.push_back(std::move(m));
  };
  auto init_p = [&](Process::ProcessModel* elt) {
    MetadataUndoRedo<Process::ProcessModel, MetadataType> m;
    m.path = score::IDocument::path(*elt);
    m.before = read(*elt);
    command.metadatas.processes.push_back(std::move(m));
  };

  for(QObject* obj : s)
  {
    if(auto itv = qobject_cast<Scenario::IntervalModel*>(obj))
      init_itv(itv);
    else if(auto e = qobject_cast<Scenario::EventModel*>(obj))
      init_ev(e);
    else if(auto ts = qobject_cast<Scenario::TimeSyncModel*>(obj))
      init_s(ts);
    else if(auto p = qobject_cast<Process::ProcessModel*>(obj))
      init_p(p);
  }
}

SetSyncMode::SetSyncMode(
    NetworkDocumentPlugin& plug, const Selection& s, SyncMode newMode)
    : m_after{newMode}
{
  setup_init_metadata<Process::NetworkFlags>(
      *this, s, [](const auto& elt) { return elt.networkFlags(); });
}

void SetSyncMode::undo(const score::DocumentContext& ctx) const
{
  for(auto& elt : metadatas.intervals)
  {
    elt.path.find(ctx).setNetworkFlags(elt.before);
  }
  for(auto& elt : metadatas.events)
  {
    elt.path.find(ctx).setNetworkFlags(elt.before);
  }
  for(auto& elt : metadatas.nodes)
  {
    elt.path.find(ctx).setNetworkFlags(elt.before);
  }
  for(auto& elt : metadatas.processes)
  {
    elt.path.find(ctx).setNetworkFlags(elt.before);
  }
}

static Process::NetworkFlags merge(SyncMode m, Process::NetworkFlags cur)
{
  cur = (Process::NetworkFlags)(cur & ~Process::NetworkFlags::Uncompensated);
  cur = (Process::NetworkFlags)(cur & ~Process::NetworkFlags::Compensated);
  cur = (Process::NetworkFlags)(cur & ~Process::NetworkFlags::Async);
  cur = (Process::NetworkFlags)(cur & ~Process::NetworkFlags::Sync);

  switch(m)
  {
    case SyncMode::NonCompensatedSync:
      cur |= Process::NetworkFlags::Uncompensated;
      cur |= Process::NetworkFlags::Sync;
      break;
    case SyncMode::NonCompensatedAsync:
      cur |= Process::NetworkFlags::Uncompensated;
      cur |= Process::NetworkFlags::Async;
      break;
    case SyncMode::CompensatedSync:
      cur |= Process::NetworkFlags::Compensated;
      cur |= Process::NetworkFlags::Sync;
      break;
    case SyncMode::CompensatedAsync:
      cur |= Process::NetworkFlags::Compensated;
      cur |= Process::NetworkFlags::Async;
      break;
  }
  return cur;
}
void SetSyncMode::redo(const score::DocumentContext& ctx) const
{
  for(auto& elt : metadatas.intervals)
  {
    auto& e = elt.path.find(ctx);
    e.setNetworkFlags(merge(m_after, e.networkFlags()));
  }
  for(auto& elt : metadatas.events)
  {
    auto& e = elt.path.find(ctx);
    e.setNetworkFlags(merge(m_after, e.networkFlags()));
  }
  for(auto& elt : metadatas.nodes)
  {
    auto& e = elt.path.find(ctx);
    e.setNetworkFlags(merge(m_after, e.networkFlags()));
  }
  for(auto& elt : metadatas.processes)
  {
    auto& e = elt.path.find(ctx);
    e.setNetworkFlags(merge(m_after, e.networkFlags()));
  }
}

void SetSyncMode::serializeImpl(DataStreamInput& s) const
{
  s << metadatas.intervals << metadatas.events << metadatas.nodes << metadatas.processes
    << m_after;
}
void SetSyncMode::deserializeImpl(DataStreamOutput& s)
{
  s >> metadatas.intervals >> metadatas.events >> metadatas.nodes >> metadatas.processes
      >> m_after;
}
SetShareMode::SetShareMode(
    NetworkDocumentPlugin& plug, const Selection& s, ShareMode newMode)
    : m_after{newMode}
{
  setup_init_metadata<Process::NetworkFlags>(
      *this, s, [](const auto& elt) { return elt.networkFlags(); });
}

void SetShareMode::undo(const score::DocumentContext& ctx) const
{
  for(auto& elt : metadatas.intervals)
  {
    elt.path.find(ctx).setNetworkFlags(elt.before);
  }
  for(auto& elt : metadatas.events)
  {
    elt.path.find(ctx).setNetworkFlags(elt.before);
  }
  for(auto& elt : metadatas.nodes)
  {
    elt.path.find(ctx).setNetworkFlags(elt.before);
  }
  for(auto& elt : metadatas.processes)
  {
    elt.path.find(ctx).setNetworkFlags(elt.before);
  }
}

static Process::NetworkFlags merge(ShareMode m, Process::NetworkFlags cur)
{
  cur = (Process::NetworkFlags)(cur & ~Process::NetworkFlags::Free);
  cur = (Process::NetworkFlags)(cur & ~Process::NetworkFlags::Shared);
  cur = (Process::NetworkFlags)(cur & ~Process::NetworkFlags::Mixed);

  switch(m)
  {
    case ShareMode::Free:
      cur |= Process::NetworkFlags::Free;
      break;
    case ShareMode::Shared:
      cur |= Process::NetworkFlags::Shared;
      break;
    case ShareMode::Mixed:
      cur |= Process::NetworkFlags::Mixed;
      break;
  }
  return cur;
}

void SetShareMode::redo(const score::DocumentContext& ctx) const
{
  auto& plug = ctx.plugin<NetworkDocumentPlugin>();
  for(auto& elt : metadatas.intervals)
  {
    auto& e = elt.path.find(ctx);
    e.setNetworkFlags(merge(m_after, e.networkFlags()));
  }
  for(auto& elt : metadatas.events)
  {
    auto& e = elt.path.find(ctx);
    e.setNetworkFlags(merge(m_after, e.networkFlags()));
  }
  for(auto& elt : metadatas.nodes)
  {
    auto& e = elt.path.find(ctx);
    e.setNetworkFlags(merge(m_after, e.networkFlags()));
  }
  for(auto& elt : metadatas.processes)
  {
    auto& e = elt.path.find(ctx);
    e.setNetworkFlags(merge(m_after, e.networkFlags()));
  }
}

void SetShareMode::serializeImpl(DataStreamInput& s) const
{
  s << metadatas.intervals << metadatas.events << metadatas.nodes << metadatas.processes
    << m_after;
}
void SetShareMode::deserializeImpl(DataStreamOutput& s)
{
  s >> metadatas.intervals >> metadatas.events >> metadatas.nodes >> metadatas.processes
      >> m_after;
}

SetGroup::SetGroup(NetworkDocumentPlugin& plug, const Selection& s, QString newMode)
    : m_after{newMode}
{
  setup_init_metadata<QString>(
      *this, s, [](const auto& elt) { return elt.networkGroup(); });
}

void SetGroup::undo(const score::DocumentContext& ctx) const
{
  for(auto& elt : metadatas.intervals)
  {
    elt.path.find(ctx).setNetworkGroup(elt.before);
  }
  for(auto& elt : metadatas.events)
  {
    elt.path.find(ctx).setNetworkGroup(elt.before);
  }
  for(auto& elt : metadatas.nodes)
  {
    elt.path.find(ctx).setNetworkGroup(elt.before);
  }
  for(auto& elt : metadatas.processes)
  {
    elt.path.find(ctx).setNetworkGroup(elt.before);
  }
}

void SetGroup::redo(const score::DocumentContext& ctx) const
{
  for(auto& elt : metadatas.intervals)
  {
    elt.path.find(ctx).setNetworkGroup(m_after);
  }
  for(auto& elt : metadatas.events)
  {
    elt.path.find(ctx).setNetworkGroup(m_after);
  }
  for(auto& elt : metadatas.nodes)
  {
    elt.path.find(ctx).setNetworkGroup(m_after);
  }
  for(auto& elt : metadatas.processes)
  {
    elt.path.find(ctx).setNetworkGroup(m_after);
  }
}

void SetGroup::serializeImpl(DataStreamInput& s) const
{
  s << metadatas.intervals << metadatas.events << metadatas.nodes << metadatas.processes
    << m_after;
}

void SetGroup::deserializeImpl(DataStreamOutput& s)
{
  s >> metadatas.intervals >> metadatas.events >> metadatas.nodes >> metadatas.processes
      >> m_after;
}

}
