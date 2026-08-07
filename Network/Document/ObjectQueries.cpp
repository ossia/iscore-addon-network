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

//! Where an object is, in terms both peers agree on.
//!
//! Not the object's own ObjectPath: that identifies by (objectName, id), and a
//! stand-in is named "OpaqueProcess" rather than after the process it replaces.
//! So the path a peer without the factory builds names something the other one
//! does not have -- and looking it up there hits a breakpoint before it ever
//! throws, killing the host.
//!
//! The interval is a real object on both sides and the id is assigned by the
//! command, so the pair is stable wherever it is read.
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

    // Checked before deserializing rather than caught after: rapidjson's type
    // checks are assertions and vanish in release, so a path of the wrong
    // shape would be read as one anyway -- on the machine running the score.
    if(!isWireObjectPath(*intervalParam))
      throw std::runtime_error{"object.state: that is not a path"};

    ObjectPath path;
    {
      JSONObject::Deserializer des{*intervalParam};
      des.writeTo(path);
    }

    // try_find: a path that names nothing is a disagreement to report, not a
    // programming error. find() breakpoints before it throws, which on a host
    // with no debugger attached is a SIGTRAP and the end of the session.
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

namespace
{
//! Put the peer's version of a process in place of the one we made.
//!
//! A stand-in can simply be told its state. A real process cannot: it was
//! built from creation data that described the other machine, so what is here
//! is the wrong object rather than an empty one, and it has to be replaced by
//! the peer's -- same id, same interval, so cables and paths still name it.
void applyState(Process::ProcessModel& proc, const rapidjson::Value& state,
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

  // The uuid says which factory to rebuild with; anything but a string here is
  // a peer we cannot understand, not a process we should guess at.
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

  // Where it sat in the rack. Removing a process takes its layer out of every
  // slot it was in, and adding one puts it in none -- so without this the slot
  // the drop just made is left empty, which is not a state the document is
  // allowed to be in: ScenarioValidityChecker asserts frontProcess on the next
  // command, and the session diverges on the edit after the drop.
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
}

void fillStandIns(
    RpcChannel& rpc, const score::DocumentContext& ctx, const Id<Client>& peer)
{
  auto& pending = Process::awaitingRemoteState();
  if(pending.empty())
    return;

  const auto todo = std::move(pending);
  pending.clear();

  for(const auto& weak : todo)
  {
    if(!weak)
      continue;

    // Captured weakly: the answer arrives later, and by then the object may
    // have been removed -- by an undo of the very command that made it.
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
        applyState(*target, result, ctx);
        },
        [](const QString& err) {
      qDebug() << "Could not fetch the state of a process this build cannot "
                  "make:"
               << err;
        });
  }
}
}
