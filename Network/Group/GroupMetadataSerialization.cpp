
#include "GroupMetadata.hpp"

#include <score/model/Identifier.hpp>
#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONValueVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>
#include <score/tools/std/Optional.hpp>

#include <QJsonObject>
#include <QJsonValue>

#include <algorithm>

/* TODO
template<>
void DataStreamReader::read(
        const Network::GroupMetadata& elt)
{
    readFrom(elt.group());
    insertDelimiter();
}

template<>
void DataStreamWriter::write(
        Network::GroupMetadata& elt)
{
    Id<Network::Group> id;
    m_stream >> id;
    elt.setGroup(id);

    checkDelimiter();
}


template<>
void JSONReader::read(
        const Network::GroupMetadata& elt)
{
    obj["Group"] = toJsonValue(elt.group());
}

template<>
void JSONWriter::write(
        Network::GroupMetadata& elt)
{
    elt.setGroup(fromJsonValue<Id<Network::Group>>(obj["Group"]));
}
*/
