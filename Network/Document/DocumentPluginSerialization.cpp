#include "DocumentPlugin.hpp"
#include "PlaceholderPolicy.hpp"

#include <score/tools/Bind.hpp>
#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>

#include <Network/Group/GroupManager.hpp>
#include <Network/Session/Session.hpp>

template <>
void DataStreamReader::read(const Network::NetworkDocumentPlugin& elt)
{
  readFrom(elt.groupManager());
  readFrom(elt.policy());

  // Note : we do not save the policy since it will be different on each
  // computer.
  insertDelimiter();
}

template <>
void DataStreamWriter::write(Network::NetworkDocumentPlugin& elt)
{
  elt.m_groups = new Network::GroupManager{*this, &elt};
  elt.m_policy = new Network::PlaceholderEditionPolicy{*this, &elt};

  checkDelimiter();
}

template <>
void JSONReader::read(const Network::NetworkDocumentPlugin& elt)
{
  obj["Groups"] = elt.groupManager();
  obj["Policy"] = elt.policy();
}

template <>
void JSONWriter::write(Network::NetworkDocumentPlugin& elt)
{
  {
    JSONWriter w(obj["Groups"]);
    elt.m_groups = new Network::GroupManager{w, &elt};
  }
  {
    JSONWriter w(obj["Policy"]);
    elt.m_policy = new Network::PlaceholderEditionPolicy{w, &elt};
  }
}

template <>
void DataStreamReader::read(const Network::ObjectMetadata& elt)
{
  m_stream << elt.syncmode << elt.sharemode << elt.group;
  insertDelimiter();
}

template <>
void DataStreamWriter::write(Network::ObjectMetadata& elt)
{
  m_stream >> elt.syncmode >> elt.sharemode >> elt.group;
  checkDelimiter();
}

template <>
void JSONReader::read(const Network::ObjectMetadata& elt)
{
  stream.StartObject();
  obj["Sync"] = (int)elt.syncmode;
  obj["Share"] = (int)elt.sharemode;
  obj["Group"] = elt.group;
  stream.EndObject();
}

template <>
void JSONWriter::write(Network::ObjectMetadata& elt)
{
  elt.syncmode = (Network::SyncMode) obj["Sync"].toInt();
  elt.sharemode = (Network::ShareMode) obj["Share"].toInt();
  elt.group = obj["Group"].toString();
}
