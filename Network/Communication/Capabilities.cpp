#include "Capabilities.hpp"

#include <Device/Protocol/ProtocolFactoryInterface.hpp>
#include <Device/Protocol/ProtocolList.hpp>

#include <Process/ProcessList.hpp>

#include <score/application/ApplicationComponents.hpp>
#include <score/application/ApplicationContext.hpp>
#include <score/plugins/UuidKey.hpp>

#include <QDataStream>
#include <QObject>

#include <algorithm>

namespace Network
{
namespace
{
QByteArrayList sorted(QByteArrayList in)
{
  std::sort(in.begin(), in.end());
  in.erase(std::unique(in.begin(), in.end()), in.end());
  return in;
}

//! Entries of `theirs` that are not in `ours`. Both are sorted.
QByteArrayList difference(const QByteArrayList& ours, const QByteArrayList& theirs)
{
  QByteArrayList res;
  std::set_difference(
      theirs.begin(), theirs.end(), ours.begin(), ours.end(), std::back_inserter(res));
  return res;
}
}

Capabilities Capabilities::local(const score::ApplicationContext& ctx)
{
  Capabilities c;

  QByteArrayList protocols;
  for(auto& proto : ctx.interfaces<Device::ProtocolFactoryList>())
    protocols.push_back(score::uuids::toByteArray(proto.concreteKey().impl()));
  c.protocols = sorted(std::move(protocols));

  QByteArrayList processes;
  for(auto& proc : ctx.interfaces<Process::ProcessFactoryList>())
    processes.push_back(score::uuids::toByteArray(proc.concreteKey().impl()));
  c.processes = sorted(std::move(processes));

  QByteArrayList commands;
  for(const auto& [group, key] : ctx.components.availableCommands())
    commands.push_back(
        QByteArray::fromStdString(group.toString()) + '/'
        + QByteArray::fromStdString(key.toString()));
  c.commands = sorted(std::move(commands));

  return c;
}

Capabilities Capabilities::lacking(const Capabilities& theirs) const
{
  Capabilities res;
  res.protocols = difference(protocols, theirs.protocols);
  res.processes = difference(processes, theirs.processes);
  res.commands = difference(commands, theirs.commands);
  return res;
}

bool Capabilities::isEmpty() const noexcept
{
  return protocols.isEmpty() && processes.isEmpty() && commands.isEmpty();
}

QString Capabilities::summary() const
{
  if(isEmpty())
    return {};

  QStringList parts;
  if(!protocols.isEmpty())
    parts += QObject::tr("%1 protocols").arg(protocols.size());
  if(!processes.isEmpty())
    parts += QObject::tr("%1 processes").arg(processes.size());
  if(!commands.isEmpty())
    parts += QObject::tr("%1 commands").arg(commands.size());
  return parts.join(QStringLiteral(", "));
}

QDataStream& operator<<(QDataStream& s, const Capabilities& c)
{
  s << c.protocols << c.processes << c.commands;
  return s;
}

QDataStream& operator>>(QDataStream& s, Capabilities& c)
{
  s >> c.protocols >> c.processes >> c.commands;
  return s;
}
}
