#include "RemoteCommand.hpp"

#include <score/application/ApplicationComponents.hpp>
#include <score/application/GUIApplicationContext.hpp>
#include <score/command/CommandData.hpp>
#include <score/document/DocumentContext.hpp>
#include <score/serialization/DataStreamVisitor.hpp>

#include <core/command/CommandStack.hpp>
#include <core/document/Document.hpp>

#include <QByteArray>
#include <QDebug>

#include <Network/Document/DocumentPlugin.hpp>

#include <exception>

namespace Network
{
namespace
{
bool fail(
    const score::DocumentContext& ctx, OnCommandFailure onFailure, const QString& reason)
{
  if(onFailure == OnCommandFailure::Decline)
  {
    qWarning() << "Declined a command from a peer:" << reason;
    return false;
  }

  if(auto* plug = ctx.findPlugin<NetworkDocumentPlugin>())
    plug->setDiverged(reason);
  else
    qWarning() << "Network session diverged:" << reason;
  return false;
}
}

bool canApplyRemoteEdit(const score::DocumentContext& ctx)
{
  auto* plug = ctx.findPlugin<NetworkDocumentPlugin>();
  return !plug || !plug->diverged();
}

bool applyRemoteCommand(
    const score::DocumentContext& ctx, const QByteArray& data,
    OnCommandFailure onFailure)
{
  if(!canApplyRemoteEdit(ctx))
    return false;

  score::CommandData cmd;
  try
  {
    DataStreamWriter writer{data};
    writer.writeTo(cmd);
  }
  catch(const std::exception& e)
  {
    return fail(
        ctx, onFailure,
        QStringLiteral("a command from another peer could not be read: %1")
            .arg(QString::fromUtf8(e.what())));
  }
  catch(...)
  {
    return fail(
        ctx, onFailure,
        QStringLiteral("a command from another peer could not be read"));
  }

  auto* command = ctx.app.instantiateUndoCommandIfAvailable(cmd);
  if(!command)
  {
    return fail(
        ctx, onFailure,
        QStringLiteral("command %1 / %2 is not available in this build")
            .arg(QString::fromUtf8(cmd.parentKey.toString()))
            .arg(QString::fromUtf8(cmd.commandKey.toString())));
  }

  try
  {
    ctx.document.commandStack().redoAndPushQuiet(command);
  }
  catch(const std::exception& e)
  {
    return fail(
        ctx, onFailure,
        QStringLiteral("command %1 could not be applied: %2")
            .arg(QString::fromUtf8(cmd.commandKey.toString()))
            .arg(QString::fromUtf8(e.what())));
  }
  catch(...)
  {
    return fail(
        ctx, onFailure,
        QStringLiteral("command %1 could not be applied")
            .arg(QString::fromUtf8(cmd.commandKey.toString())));
  }

  return true;
}
}
