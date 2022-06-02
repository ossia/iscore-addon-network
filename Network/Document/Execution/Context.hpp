#pragma once
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Group/Group.hpp>
#include <Network/Group/GroupManager.hpp>

namespace Network
{

struct Constants
{
  static const Constants& instance()
  {
    static const Constants c;
    return c;
  }
  const QString syncmode{QStringLiteral("syncmode")};
  const QString async_compensated{QStringLiteral("async_compensated")};
  const QString async_uncompensated{QStringLiteral("async_uncompensated")};
  const QString sync_compensated{QStringLiteral("sync_compensated")};
  const QString sync_uncompensated{QStringLiteral("sync_uncompensated")};

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


template <typename T>
SyncMode getInfos(NetworkDocumentPlugin& doc, const T& obj)
{
  if(const ObjectMetadata* meta = doc.get_metadata(obj))
    return meta->syncmode;
  else
  return SyncMode::NonCompensatedAsync;
}

template <typename T>
const Group& getGroup(NetworkDocumentPlugin& doc, const GroupManager& gm, const Group& cur, const T& obj)
{
  const ObjectMetadata* meta = doc.get_metadata(obj);
  const Group* cur_group = &cur;
  if(!meta)
    return *cur_group;

  auto& str = meta->group;
  const auto& cst = Constants::instance();
  if (str == cst.all)
  {
    cur_group = gm.group(gm.defaultGroup());
  }
  else if (str == cst.parent || str.isEmpty())
  {
    // Default
  }
  else
  {
    // look for a group of this name
    auto group = gm.findGroup(str);
    if (group)
    {
      cur_group = group; // Else we default to the "parent" case.
    }
  }
  SCORE_ASSERT(cur_group);
  return *cur_group;
}
}

namespace Execution
{
class IntervalComponent;
class TimeSyncComponent;
struct Context;
class ProcessComponent;
}
