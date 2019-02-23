
#include "Group.hpp"

#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONValueVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>

#include <QDataStream>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QtGlobal>

#include <algorithm>

template <typename V>
void fromJsonValueArray(const QJsonArray& json_arr, QVector<Id<V>>& arr)
{
  arr.reserve(json_arr.size());
  for (const auto& elt : json_arr)
  {
    arr.push_back(fromJsonValue<Id<V>>(elt));
  }
}

template <>
void DataStreamReader::read(const Network::Group& elt)
{
  m_stream << elt.name() << elt.clients();
  insertDelimiter();
}

template <>
void DataStreamWriter::write(Network::Group& elt)
{
  m_stream >> elt.m_name >> elt.m_executingClients;
  checkDelimiter();
}

template <>
void JSONObjectReader::read(const Network::Group& elt)
{
  obj[strings.Name] = elt.name();
  obj["Clients"] = toJsonArray(elt.clients());
}

template <>
void JSONObjectWriter::write(Network::Group& elt)
{
  elt.m_name = obj[strings.Name].toString();
  fromJsonValueArray(obj["Clients"].toArray(), elt.m_executingClients);
}
