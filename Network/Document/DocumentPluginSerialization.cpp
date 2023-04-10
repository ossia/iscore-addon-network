#include "DocumentPlugin.hpp"
#include "PlaceholderPolicy.hpp"

#include <Scenario/Process/ScenarioModel.hpp>

#include <score/model/path/PathSerialization.hpp>
#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>
#include <score/tools/Bind.hpp>

#include <Network/Group/GroupManager.hpp>
#include <Network/Session/Session.hpp>

template <typename T>
void readMetadataMap(
    DataStreamInput& s, const std::unordered_map<const T*, Network::ObjectMetadata>& map)
{
  s << (int64_t)map.size();
  for(const auto& [itv, meta] : map)
  {
    s << Path<T>{*itv} << meta;
  }
}

template <typename T>
void writeMetadataMap(
    DataStreamOutput& s, std::vector<std::pair<Path<T>, Network::ObjectMetadata>>& map)
{
  int64_t sz{};
  s >> sz;
  for(int64_t i = 0; i < sz; ++i)
  {
    Path<T> p;
    Network::ObjectMetadata m;
    s >> p >> m;

    map.emplace_back(std::move(p), std::move(m));
  }
}

template <typename T>
void readMetadataMap(
    JSONReader& s, const std::unordered_map<const T*, Network::ObjectMetadata>& map)
{
  s.stream.StartArray();
  for(const auto& [itv, meta] : map)
  {
    s.stream.StartObject();
    s.obj["Path"] = Path<T>{*itv};
    s.obj["Data"] = meta;
    s.stream.EndObject();
  }
  s.stream.EndArray();
}

template <typename T>
void writeMetadataMap(
    const JsonValue& s, std::vector<std::pair<Path<T>, Network::ObjectMetadata>>& map)
{
  for(auto& obj : s.toArray())
  {
    auto val = JsonValue{obj};
    map.emplace_back(
        val["Path"].to<Path<T>>(), val["Data"].to<Network::ObjectMetadata>());
  }
}

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
