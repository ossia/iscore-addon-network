
#include "Client.hpp"

#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include <wobjectimpl.h>
W_OBJECT_IMPL(Network::Client)
template <>
void DataStreamReader::read(const Network::Client& elt)
{
  m_stream << elt.name();
  insertDelimiter();
}

template <>
void DataStreamWriter::write(Network::Client& elt)
{
  QString s;
  m_stream >> s;
  elt.setName(s);

  checkDelimiter();
}

template <>
void JSONReader::read(const Network::Client& elt)
{
  obj[strings.Name] = elt.name();
}

template <>
void JSONWriter::write(Network::Client& elt)
{
  elt.setName(obj[strings.Name].toString());
}
