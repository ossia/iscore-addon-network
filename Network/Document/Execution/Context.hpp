#pragma once
#include <Network/Document/DocumentPlugin.hpp>

#include <Network/Group/Group.hpp>
#include <Network/Group/GroupManager.hpp>

namespace Network
{

struct Constants
{
    static const Constants& instance() { static const Constants c; return c; }
    const QString syncmode{QStringLiteral("syncmode")};
    const QString async{QStringLiteral("async")};
    const QString sync{QStringLiteral("sync")};

    const QString order{QStringLiteral("order")};
    const QString ordered{QStringLiteral("true")};
    const QString unordered{QStringLiteral("false")};

    const QString group{QStringLiteral("group")};
    const QString parent{QStringLiteral("parent")};
    const QString all{QStringLiteral("all")};

    const QString sharemode{QStringLiteral("sharemode")};
    const QString shared{QStringLiteral("shared")};
    const QString mixed{QStringLiteral("mixed")};
    const QString free{QStringLiteral("free")};
};

struct NetworkPrunerContext
{
  NetworkDocumentPlugin& doc;
  Session& session;
  GroupManager& gm;
  const Id<Client> self;
  const Id<Client> master;

  const MessagesAPI& mapi = MessagesAPI::instance();
};


template<typename T, typename Obj>
optional<T> get_metadata(Obj& obj, const QString& s)
{
  auto& m = obj.metadata().getExtendedMetadata();
  auto it = m.constFind(s);
  if(it != m.constEnd())
  {
    const QVariant& var = *it;
    if(var.canConvert<T>())
      return var.value<T>();
  }
  return {};
}

template<typename T>
SyncMode getInfos(const T& obj)
{
  const auto& str = Constants::instance();

  auto syncmode = get_metadata<QString>(obj, str.syncmode);
  if(!syncmode || syncmode->isEmpty())
    syncmode = QString(str.async);
  auto order = get_metadata<QString>(obj, str.order);
  if(!order || order->isEmpty())
    order = str.unordered;

  if(syncmode == str.async && order == str.ordered)
    return SyncMode::NonCompensatedSync;
  else if(syncmode == str.async && order == str.unordered)
    return SyncMode::NonCompensatedAsync;
  else if(syncmode == str.sync && order == str.ordered)
    return SyncMode::CompensatedSync;
  else if(syncmode == str.sync && order == str.unordered)
    return SyncMode::CompensatedAsync;

  return SyncMode::NonCompensatedAsync;
}

template<typename T>
const Group& getGroup(const GroupManager& gm, const Group& cur, const T& obj)
{
  const auto& cst = Constants::instance();

  const Group* cur_group = &cur;
  auto ostr = get_metadata<QString>(obj, cst.group);
  if(!ostr)
    return *cur_group;

  auto& str = *ostr;
  if(str == cst.all)
  {
    cur_group = gm.group(gm.defaultGroup());
  }
  else if(str == cst.parent || str.isEmpty())
  {
    // Default
  }
  else
  {
    // look for a group of this name
    auto group = gm.findGroup(str);
    if(group)
    {
      cur_group = group; // Else we default to the "parent" case.
    }
  }
  SCORE_ASSERT(cur_group);
  return *cur_group;
}
}

namespace Engine
{
namespace Execution
{
class IntervalComponent;
class TimeSyncComponent;
struct Context;
class ProcessComponent;
}
}
