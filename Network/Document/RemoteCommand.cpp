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
#include <QObject>

#include <Network/Document/DocumentPlugin.hpp>

#include <exception>
#include <memory>

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

//! A command that threw part-way through has already changed the model, and
//! nothing puts that back. Whichever side we are, we no longer match the rest
//! of the session -- declining is not available once the damage is done.
bool failedWhileApplying(
    const score::DocumentContext& ctx, const score::CommandData& cmd,
    const QString& why)
{
  return fail(
      ctx, OnCommandFailure::MarkDiverged,
      QStringLiteral("command %1 failed part-way through: %2")
          .arg(QString::fromUtf8(cmd.commandKey.toString()))
          .arg(why));
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

  std::unique_ptr<score::Command> command{
      ctx.app.instantiateUndoCommandIfAvailable(cmd)};
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
    ctx.document.commandStack().redoAndPushQuiet(command.release());
  }
  catch(const std::exception& e)
  {
    return failedWhileApplying(
        ctx, cmd, QString::fromUtf8(e.what()));
  }
  catch(...)
  {
    return failedWhileApplying(ctx, cmd, QObject::tr("it could not be applied"));
  }

  return true;
}
}
