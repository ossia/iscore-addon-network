#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>

#include <Network/Group/GroupManager.hpp>
#include <Network/Session/Session.hpp>
#include "DocumentPlugin.hpp"
#include "PlaceholderPolicy.hpp"

template <>
void DataStreamReader::read(
        const Network::NetworkDocumentPlugin& elt)
{
    readFrom(elt.groupManager());
    readFrom(elt.policy());

    // Note : we do not save the policy since it will be different on each computer.
    insertDelimiter();
}

template <>
void DataStreamWriter::write(
        Network::NetworkDocumentPlugin& elt)
{
    elt.m_groups = new Network::GroupManager{*this, &elt};
    elt.m_policy = new Network::PlaceholderEditionPolicy{*this, &elt};

    checkDelimiter();
}

template <>
void JSONObjectReader::read(
        const Network::NetworkDocumentPlugin& elt)
{
    obj["Groups"] = toJsonObject(elt.groupManager());
    obj["Policy"] = toJsonObject(elt.policy());
}

template <>
void JSONObjectWriter::write(
        Network::NetworkDocumentPlugin& elt)
{
  {
    JSONObjectWriter w(obj["Groups"].toObject());
    elt.m_groups = new Network::GroupManager{w, &elt};
  }
  {
    JSONObjectWriter w(obj["Policy"].toObject());
    elt.m_policy = new Network::PlaceholderEditionPolicy{w, &elt};
  }
}
