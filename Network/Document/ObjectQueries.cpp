#include "ObjectQueries.hpp"

#include <Process/OpaqueProcess.hpp>
#include <Process/ProcessList.hpp>
#include <Process/RemoteState.hpp>

#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Process/Algorithms/ProcessPolicy.hpp>

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

#include <stdexcept>

namespace Network
{
namespace
{
constexpr auto object_state = "object.state";

QByteArray pathParams(const ObjectPath& path)
{
  JSONReader r;
  r.readFrom(path);

  rapidjson::StringBuffer buf;
  JsonWriter w{buf};
  w.StartObject();
  w.Key("path");
  {
    rapidjson::Document d;
    const auto bytes = r.toByteArray();
    d.Parse(bytes.data(), bytes.size());
    d.Accept(w);
  }
  w.EndObject();
  return QByteArray{buf.GetString(), (int)buf.GetLength()};
}
}

void bindObjectQueries(RpcChannel& rpc, const score::DocumentContext& ctx)
{
  rpc.bind(object_state, [&ctx](const rapidjson::Value& params) -> QByteArray {
    if(!params.IsObject() || !params.HasMember("path"))
      throw std::runtime_error{"object.state: no path"};

    ObjectPath path;
    {
      JSONObject::Deserializer des{params["path"]};
      des.writeTo(path);
    }

    // find() throws when the path names nothing, or names something of another
    // type -- both of which mean the peers disagree about the document, which
    // is worth reporting as an error rather than answering with silence.
    auto& obj = path.find<QObject>(ctx);
    auto* proc = qobject_cast<Process::ProcessModel*>(&obj);
    if(!proc)
      throw std::runtime_error{"object.state: not a process"};

    JSONReader r;
    r.readFrom(*proc);
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
  if(!itv || !state.IsObject() || !state.HasMember(score::StringConstant().uuid))
    return;

  const JsonValue obj{state};
  const auto key = obj[score::StringConstant().uuid]
                       .to<UuidKey<Process::ProcessModel>>();

  auto& facs = ctx.app.interfaces<Process::ProcessFactoryList>();
  auto* fac = facs.get(key);

  JSONObject::Deserializer des{state};
  auto* rebuilt = fac ? fac->load(des.toVariant(), ctx, itv)
                      : facs.loadMissing(key, des.toVariant(), ctx, itv);
  if(!rebuilt)
    return;

  Scenario::RemoveProcess(*itv, proc.id());
  Scenario::AddProcess(*itv, rebuilt);
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
    const auto path = score::IDocument::path(*weak).unsafePath();

    rpc.call(
        peer, object_state, pathParams(path),
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
