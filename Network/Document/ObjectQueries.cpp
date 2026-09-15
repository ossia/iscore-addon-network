#include "ObjectQueries.hpp"

#include <Process/OpaqueProcess.hpp>
#include <Process/ProcessList.hpp>
#include <Process/RemoteState.hpp>

#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/Interval/Slot.hpp>
#include <Scenario/Process/Algorithms/ProcessPolicy.hpp>

#include <ossia/detail/algorithms.hpp>

#include <score/application/ApplicationContext.hpp>
#include <score/plugins/StringFactoryKey.hpp>
#include <score/serialization/JSONValueVisitor.hpp>
#include <Process/Process.hpp>

#include <score/document/DocumentContext.hpp>
#include <score/document/DocumentInterface.hpp>
#include <score/model/path/ObjectPath.hpp>
#include <score/serialization/JSONVisitor.hpp>
#include <score/model/EntitySerialization.hpp>
#include <score/plugins/SerializableHelpers.hpp>

#include <QPointer>

#include <Network/Communication/Rpc.hpp>
#include <Network/Communication/WireJson.hpp>

#include <stdexcept>

namespace Network
{
namespace
{
constexpr auto object_state = "object.state";

//! Where an object is, in terms both peers agree on: interval path plus
//! process id. Not the process's own ObjectPath, which names a stand-in
//! "OpaqueProcess" and so resolves to nothing on the other side.
QByteArray processParams(const ObjectPath& interval, int32_t process)
{
  JSONReader r;
  r.readFrom(interval);

  rapidjson::StringBuffer buf;
  JsonWriter w{buf};
  w.StartObject();
  w.Key("interval");
  {
    rapidjson::Document d;
    const auto bytes = r.toByteArray();
    d.Parse(bytes.data(), bytes.size());
    d.Accept(w);
  }
  w.Key("process");
  w.Int(process);
  w.EndObject();
  return QByteArray{buf.GetString(), (int)buf.GetLength()};
}
}

void bindObjectQueries(RpcChannel& rpc, const score::DocumentContext& ctx)
{
  rpc.bind(object_state, [&ctx](const rapidjson::Value& params) -> QByteArray {
    const auto* intervalParam = wireMember(params, "interval");
    const auto processParam = wireInt(params, "process");
    if(!intervalParam || !processParam)
      throw std::runtime_error{"object.state: no object named"};

    // Checked, not caught: rapidjson's type checks vanish in release.
    if(!isWireObjectPath(*intervalParam))
      throw std::runtime_error{"object.state: that is not a path"};

    ObjectPath path;
    {
      JSONObject::Deserializer des{*intervalParam};
      des.writeTo(path);
    }

    // try_find: find() breakpoints before it throws, which kills the host.
    auto* itv = path.try_find<Scenario::IntervalModel>(ctx);
    if(!itv)
      throw std::runtime_error{"object.state: no such interval here"};

    const Id<Process::ProcessModel> id{(int32_t)*processParam};
    auto it = ossia::find_if(
        itv->processes, [&](const Process::ProcessModel& p) { return p.id() == id; });
    if(it == itv->processes.end())
      throw std::runtime_error{"object.state: no such process here"};

    JSONReader r;
    r.readFrom(*it);
    return r.toByteArray();
  });
}

//! Put the peer's version of a process in place of the one we made. A
//! stand-in is told its state; a real one is replaced, keeping id and interval
//! so cables and paths still name it.
void applyRemoteProcessState(
    Process::ProcessModel& proc, const rapidjson::Value& state,
    const score::DocumentContext& ctx)
{
  if(auto* opaque = qobject_cast<Process::OpaqueProcessModel*>(&proc))
  {
    opaque->setState(state);
    return;
  }

  auto* itv = qobject_cast<Scenario::IntervalModel*>(proc.parent());
  if(!itv)
    return;

  // Which factory to rebuild with.
  const auto uuid = wireString(state, score::StringConstant().uuid.c_str());
  if(!uuid)
    return;

  const auto key = UuidKey<Process::ProcessModel>::fromString(*uuid);

  auto& facs = ctx.app.interfaces<Process::ProcessFactoryList>();
  auto* fac = facs.get(key);

  JSONObject::Deserializer des{state};
  auto* rebuilt = fac ? fac->load(des.toVariant(), ctx, itv)
                      : facs.loadMissing(key, des.toVariant(), ctx, itv);
  if(!rebuilt)
    return;

  // Everything below names the process by the id we asked about, so an answer
  // about another one would leave the rack pointing at nothing.
  if(rebuilt->id() != proc.id())
  {
    delete rebuilt;
    qDebug() << "A peer answered about a process we did not ask about";
    return;
  }

  // Where it sat in the rack: removing takes the layer out of every slot and
  // adding puts it in none, which leaves a slot the document may not have.
  const auto id = proc.id();
  struct Placement
  {
    int slot{};
    bool front{};
  };
  std::vector<Placement> placements;
  {
    const auto& rack = itv->smallView();
    for(int i = 0; i < std::ssize(rack); i++)
    {
      if(rack[i].nodal || !ossia::contains(rack[i].processes, id))
        continue;
      placements.push_back({i, rack[i].frontProcess == id});
    }
  }

  Scenario::RemoveProcess(*itv, id);
  Scenario::AddProcess(*itv, rebuilt);

  for(const auto& p : placements)
  {
    if(p.slot >= std::ssize(itv->smallView()))
      continue;
    itv->addLayer(p.slot, id);
    if(p.front)
      itv->putLayerToFront(p.slot, id);
  }
}

void fillStandIns(
    RpcChannel& rpc, const score::DocumentContext& ctx, const Id<Client>& peer)
{
  auto& pending = Process::awaitingRemoteState();
  if(pending.empty())
    return;

  // Only what belongs to *this* document. The list is one per process while
  // documents are many: draining it wholesale meant one document asking its own
  // peer about another document's processes, and answering into them with the
  // wrong context. What is not ours is left for whoever it belongs to.
  auto* mine = score::IDocument::documentFromObject(ctx.document);
  std::vector<QPointer<Process::ProcessModel>> todo;
  {
    std::vector<QPointer<Process::ProcessModel>> others;
    for(const auto& weak : pending)
    {
      if(!weak)
        continue;
      if(score::IDocument::documentFromObject(*weak) == mine)
        todo.push_back(weak);
      else
        others.push_back(weak);
    }
    pending = std::move(others);
  }

  for(const auto& weak : todo)
  {
    if(!weak)
      continue;

    // Weakly: an undo can remove the object before the answer arrives.
    QPointer<Process::ProcessModel> target = weak;

    auto* itv = qobject_cast<Scenario::IntervalModel*>(weak->parent());
    if(!itv)
      continue;

    const auto intervalPath = score::IDocument::path(*itv).unsafePath();
    const auto processId = weak->id_val();

    rpc.call(
        peer, object_state, processParams(intervalPath, processId),
        [target, &ctx](const rapidjson::Value& result) {
      if(target)
        applyRemoteProcessState(*target, result, ctx);
        },
        [](const QString& err) {
      qDebug() << "Could not fetch the state of a process this build cannot "
                  "make:"
               << err;
        });
  }
}
}
