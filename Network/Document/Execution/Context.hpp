#pragma once
#include <Network/Document/DocumentPlugin.hpp>

#include <Network/Group/Group.hpp>
#include <Network/Group/GroupManager.hpp>

namespace Network
{

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
  auto syncmode = get_metadata<QString>(obj, "syncmode");
  if(!syncmode || syncmode->isEmpty())
    syncmode = QString("async");
  auto order = get_metadata<QString>(obj, "order");
  if(!order || order->isEmpty())
    order = QString("false");

  const QString async = QStringLiteral("async");
  const QString sync = QStringLiteral("async");
  const QString ordered = QStringLiteral("true");
  const QString unordered = QStringLiteral("false");

  if(syncmode == async && order == ordered)
    return SyncMode::AsyncOrdered;
  else if(syncmode == async && order == unordered)
    return SyncMode::AsyncUnordered;
  else if(syncmode == sync && order == ordered)
    return SyncMode::SyncOrdered;
  else if(syncmode == sync && order == unordered)
    return SyncMode::SyncUnordered;

  return SyncMode::AsyncUnordered;
}

template<typename T>
const Group& getGroup(const GroupManager& gm, const Group& cur, const T& obj)
{
  const Group* cur_group = &cur;

  /*
  // First look if there is a group
  auto comp = iscore::findComponent<GroupMetadata>(cst.iscoreConstraint().components());
  if(comp)
  {

  }
  else
  {
    // We assume that we keep the parent group.
  }

  // If no group found through components, maybe through metadata :
  */
  auto ostr = get_metadata<QString>(obj, "group");
  if(!ostr)
    return *cur_group;

  auto& str = *ostr;
  if(str == "all")
  {
    cur_group = gm.group(gm.defaultGroup());
  }
  else if(str == "parent" || str.isEmpty())
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
  ISCORE_ASSERT(cur_group);
  return *cur_group;
}
}

namespace Engine
{
namespace Execution
{
class ConstraintComponent;
class TimeNodeComponent;
struct Context;
class ProcessComponent;
}
}
