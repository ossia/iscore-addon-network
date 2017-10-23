
#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <sys/types.h>
#include <algorithm>
#include <vector>

#include "Group.hpp"
#include "GroupManager.hpp"

template <>
void DataStreamReader::read(
        const Network::GroupManager& elt)
{
    const auto& groups = elt.groups();
    m_stream << (int32_t) groups.size();
    for(const auto& group : groups)
    {
        readFrom(*group);
    }

    insertDelimiter();
}

template <>
void DataStreamWriter::write(
        Network::GroupManager& elt)
{
    int32_t size;
    m_stream >> size;
    for(auto i = size; i --> 0; )
    {
        elt.addGroup(new Network::Group{*this, &elt});
    }
    checkDelimiter();
}

template <>
void JSONObjectReader::read(
        const Network::GroupManager& elt)
{
    obj["GroupList"] = toJsonArray(elt.groups());
}

template <>
void JSONObjectWriter::write(
        Network::GroupManager& elt)
{
    auto arr = obj["GroupList"].toArray();
    for(const auto& json_vref : arr)
    {
        JSONObject::Deserializer deserializer {json_vref.toObject()};
        elt.addGroup(new Network::Group{deserializer, &elt});
    }
}
