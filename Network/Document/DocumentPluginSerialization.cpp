#include <iscore/serialization/DataStreamVisitor.hpp>
#include <iscore/serialization/JSONVisitor.hpp>

#include <Network/Group/GroupManager.hpp>
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
    readFrom(elt.groupManager());
    readFrom(elt.policy());
}

template <>
void JSONObjectWriter::write(
        Network::NetworkDocumentPlugin& elt)
{
    elt.m_groups = new Network::GroupManager{*this, &elt};
    elt.m_policy = new Network::PlaceholderEditionPolicy{*this, &elt};
}
