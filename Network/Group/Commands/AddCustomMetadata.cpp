
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

template <typename T>
struct TSerializer<DataStream, Network::Command::MetadataUndoRedo<T>>
{
  static void readFrom(
      DataStream::Serializer& s,
      const Network::Command::MetadataUndoRedo<T>& obj)
  {
    s.stream() << obj.path << obj.before;
  }

  static void writeTo(
      DataStream::Deserializer& s,
      Network::Command::MetadataUndoRedo<T>& obj)
  {
    s.stream() >> obj.path >> obj.before;
  }
};
namespace Network::Command
{

void update_metadata(NetworkDocumentPlugin& plug, auto& obj, SyncMode m)
{
  if(auto p = plug.get_metadata(obj))
  {
    p->syncmode = m;
  }
  else
  {
    ObjectMetadata meta;
    meta.syncmode = m;
    plug.set_metadata(obj, std::move(meta));
  }
}
void update_metadata(NetworkDocumentPlugin& plug, auto& obj, ShareMode m)
{
  if(auto p = plug.get_metadata(obj))
  {
    p->sharemode = m;
  }
  else
  {
    ObjectMetadata meta;
    meta.sharemode = m;
    plug.set_metadata(obj, std::move(meta));
  }
}
void update_metadata(NetworkDocumentPlugin& plug, auto& obj, bool ordered)
{
  if(auto p = plug.get_metadata(obj))
  {
    p->ordered = ordered;
  }
  else
  {
    ObjectMetadata meta;
    meta.ordered = ordered;
    plug.set_metadata(obj, std::move(meta));
  }
}
void update_metadata(NetworkDocumentPlugin& plug, auto& obj, QString group)
{
  if(auto p = plug.get_metadata(obj))
  {
    p->group = group;
  }
  else
  {
    ObjectMetadata meta;
    meta.group = group;
    plug.set_metadata(obj, std::move(meta));
  }
}

UpdateObjectMetadata::UpdateObjectMetadata(
    NetworkDocumentPlugin& plug, const Selection& s)
{
  auto init_itv = [&] (Scenario::IntervalModel* elt)
  {
    MetadataUndoRedo<Scenario::IntervalModel> m;
    m.path = score::IDocument::path(*elt);
    if(auto p = plug.get_metadata(*elt))
      m.before = *p;
    m_intervals.push_back(std::move(m));
  };
  auto init_ev = [&] (Scenario::EventModel* elt)
  {
    MetadataUndoRedo<Scenario::EventModel> m;
    m.path = score::IDocument::path(*elt);
    if(auto p = plug.get_metadata(*elt))
      m.before = *p;
    m_events.push_back(std::move(m));
  };
  auto init_s = [&] (Scenario::TimeSyncModel* elt)
  {
    MetadataUndoRedo<Scenario::TimeSyncModel> m;
    m.path = score::IDocument::path(*elt);
    if(auto p = plug.get_metadata(*elt))
      m.before = *p;
    m_nodes.push_back(std::move(m));
  };
  auto init_p = [&] (Process::ProcessModel* elt)
  {
    MetadataUndoRedo<Process::ProcessModel> m;
    m.path = score::IDocument::path(*elt);
    if(auto p = plug.get_metadata(*elt))
      m.before = *p;
    m_processes.push_back(std::move(m));
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

void UpdateObjectMetadata::undo(const score::DocumentContext& ctx) const
{
  auto& plug = ctx.plugin<NetworkDocumentPlugin>();

  for (auto& elt : m_intervals)
  {
    if(elt.before)
      plug.set_metadata(elt.path.find(ctx), *elt.before);
    else
      plug.unset_metadata(elt.path.find(ctx));
  }
  for (auto& elt : m_events)
  {
    if(elt.before)
      plug.set_metadata(elt.path.find(ctx), *elt.before);
    else
      plug.unset_metadata(elt.path.find(ctx));
  }
  for (auto& elt : m_nodes)
  {
    if(elt.before)
      plug.set_metadata(elt.path.find(ctx), *elt.before);
    else
      plug.unset_metadata(elt.path.find(ctx));
  }
  for (auto& elt : m_processes)
  {
    if(elt.before)
      plug.set_metadata(elt.path.find(ctx), *elt.before);
    else
      plug.unset_metadata(elt.path.find(ctx));
  }
}

SetSyncMode::SetSyncMode(
    NetworkDocumentPlugin& plug, const Selection& s,
    SyncMode newMode)
  : UpdateObjectMetadata{plug, s}
  , m_after{newMode}
{
}

void SetSyncMode::redo(const score::DocumentContext& ctx) const
{
  auto& plug = ctx.plugin<NetworkDocumentPlugin>();
  for (auto& elt : m_intervals)
  {
    update_metadata(plug, elt.path.find(ctx), m_after);
  }
  for (auto& elt : m_events)
  {
    update_metadata(plug, elt.path.find(ctx), m_after);
  }
  for (auto& elt : m_nodes)
  {
    update_metadata(plug, elt.path.find(ctx), m_after);
  }
  for (auto& elt : m_processes)
  {
    update_metadata(plug, elt.path.find(ctx), m_after);
  }
}

void SetSyncMode::serializeImpl(DataStreamInput& s) const
{
  s << m_intervals << m_events << m_nodes << m_processes << m_after;
}

void SetSyncMode::deserializeImpl(DataStreamOutput& s)
{
  s >> m_intervals >> m_events >> m_nodes >> m_processes >> m_after;
}

SetShareMode::SetShareMode(
    NetworkDocumentPlugin& plug,const Selection& s,
    ShareMode newMode)
  : UpdateObjectMetadata{plug, s}
  , m_after{newMode}
{
}

void SetShareMode::redo(const score::DocumentContext& ctx) const
{
  auto& plug = ctx.plugin<NetworkDocumentPlugin>();
  for (auto& elt : m_intervals)
  {
    update_metadata(plug, elt.path.find(ctx), m_after);
  }
  for (auto& elt : m_events)
  {
    update_metadata(plug, elt.path.find(ctx), m_after);
  }
  for (auto& elt : m_nodes)
  {
    update_metadata(plug, elt.path.find(ctx), m_after);
  }
  for (auto& elt : m_processes)
  {
    update_metadata(plug, elt.path.find(ctx), m_after);
  }
}

void SetShareMode::serializeImpl(DataStreamInput& s) const
{
  s << m_intervals << m_events << m_nodes << m_processes << m_after;
}

void SetShareMode::deserializeImpl(DataStreamOutput& s)
{
  s >> m_intervals >> m_events >> m_nodes >> m_processes >> m_after;
}

SetOrderedMode::SetOrderedMode(
    NetworkDocumentPlugin& plug,const Selection& s,
    bool newMode)
  : UpdateObjectMetadata{plug, s}
  , m_after{newMode}
{
}

void SetOrderedMode::redo(const score::DocumentContext& ctx) const
{
  auto& plug = ctx.plugin<NetworkDocumentPlugin>();
  for (auto& elt : m_intervals)
  {
    update_metadata(plug, elt.path.find(ctx), m_after);
  }
  for (auto& elt : m_events)
  {
    update_metadata(plug, elt.path.find(ctx), m_after);
  }
  for (auto& elt : m_nodes)
  {
    update_metadata(plug, elt.path.find(ctx), m_after);
  }
  for (auto& elt : m_processes)
  {
    update_metadata(plug, elt.path.find(ctx), m_after);
  }
}

void SetOrderedMode::serializeImpl(DataStreamInput& s) const
{
  s << m_intervals << m_events << m_nodes << m_processes << m_after;
}

void SetOrderedMode::deserializeImpl(DataStreamOutput& s)
{
  s >> m_intervals >> m_events >> m_nodes >> m_processes >> m_after;
}

SetGroup::SetGroup(
    NetworkDocumentPlugin& plug,const Selection& s,
    QString newMode)
  : UpdateObjectMetadata{plug, s}
  , m_after{newMode}
{
}

void SetGroup::redo(const score::DocumentContext& ctx) const
{
  auto& plug = ctx.plugin<NetworkDocumentPlugin>();
  for (auto& elt : m_intervals)
  {
    update_metadata(plug, elt.path.find(ctx), m_after);
  }
  for (auto& elt : m_events)
  {
    update_metadata(plug, elt.path.find(ctx), m_after);
  }
  for (auto& elt : m_nodes)
  {
    update_metadata(plug, elt.path.find(ctx), m_after);
  }
  for (auto& elt : m_processes)
  {
    update_metadata(plug, elt.path.find(ctx), m_after);
  }
}

void SetGroup::serializeImpl(DataStreamInput& s) const
{
  s << m_intervals << m_events << m_nodes << m_processes << m_after;
}

void SetGroup::deserializeImpl(DataStreamOutput& s)
{
  s >> m_intervals >> m_events >> m_nodes >> m_processes >> m_after;
}

}
