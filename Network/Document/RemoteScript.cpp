#include "RemoteScript.hpp"

#include <score/application/ScriptEvaluator.hpp>
#include <score/document/DocumentContext.hpp>
#include <score/serialization/JSONVisitor.hpp>

#include <core/document/Document.hpp>

#include <QObject>

#include <Network/Communication/Rpc.hpp>
#include <Network/Communication/WireJson.hpp>
#include <Network/Session/ClientSession.hpp>
#include <Network/Client/RemoteClient.hpp>
#include <Network/Document/DocumentPlugin.hpp>

namespace Network
{
namespace
{
constexpr auto script_eval = "script.eval";

QByteArray codeParams(const QString& code)
{
  const auto utf8 = code.toUtf8();
  rapidjson::StringBuffer buf;
  JsonWriter w{buf};
  w.StartObject();
  w.Key("code");
  w.String(utf8.constData(), utf8.size());
  w.EndObject();
  return QByteArray{buf.GetString(), (int)buf.GetLength()};
}
}

void bindScriptForwarding(ClientSession& session, const score::DocumentContext& ctx)
{
  ctx.document.setScriptSink(
      [&session, &ctx](const QString& code, std::function<void(QString)> onResult) {
    auto* plug = ctx.findPlugin<NetworkDocumentPlugin>();
    if(!plug || !plug->rpc())
    {
      if(onResult)
        onResult(QObject::tr("not connected to a session"));
      return;
    }

    plug->rpc()->call(
        session.master().id(), script_eval, codeParams(code),
        [onResult](const rapidjson::Value& result) {
      if(!onResult)
        return;
      onResult(wireString(result, "output").value_or(QString{}));
        },
        [onResult](const QString& err) {
      if(onResult)
        onResult(QObject::tr("the other machine did not answer: %1").arg(err));
        });
      });
}

void bindScriptEvaluation(RpcChannel& rpc, const score::DocumentContext& ctx)
{
  rpc.bind(script_eval, [&ctx](const rapidjson::Value& params) -> QByteArray {
    const auto code = wireString(params, "code");
    if(!code)
      throw std::runtime_error{"a script must be given"};

    // Nothing here knows what JavaScript is: whichever plug-in can run it says
    // so by registering, and a build without it answers plainly rather than
    // pretending the script ran.
    auto* evaluator = score::scriptEvaluator();
    const QString output
        = evaluator ? evaluator->evaluate(ctx, *code)
                    : QObject::tr("this machine cannot run scripts");

    const auto utf8 = output.toUtf8();
    rapidjson::StringBuffer buf;
    JsonWriter w{buf};
    w.StartObject();
    w.Key("output");
    w.String(utf8.constData(), utf8.size());
    w.EndObject();
    return QByteArray{buf.GetString(), (int)buf.GetLength()};
  });
}
}
