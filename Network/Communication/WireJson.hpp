#pragma once
#include <score/serialization/JSONVisitor.hpp>
#include <score/serialization/StringConstants.hpp>

#include <QString>

#include <optional>

namespace Network
{
/**
 * @brief Reading JSON another machine sent.
 *
 * rapidjson checks types with assertions, which are compiled out in release:
 * GetString() on a number then reads the union as a pointer and length. A peer
 * on a different build is the normal case here, so every accessor answers
 * "not there" rather than trusting the shape.
 */
inline const rapidjson::Value*
wireMember(const rapidjson::Value& v, const char* key) noexcept
{
  if(!v.IsObject())
    return nullptr;
  auto it = v.FindMember(key);
  return it != v.MemberEnd() ? &it->value : nullptr;
}

inline std::optional<QString>
wireString(const rapidjson::Value& v, const char* key) noexcept
{
  if(auto* m = wireMember(v, key); m && m->IsString())
    return QString::fromUtf8(m->GetString(), m->GetStringLength());
  return std::nullopt;
}

inline std::optional<bool> wireBool(const rapidjson::Value& v, const char* key) noexcept
{
  if(auto* m = wireMember(v, key); m && m->IsBool())
    return m->GetBool();
  return std::nullopt;
}

inline std::optional<int64_t>
wireInt(const rapidjson::Value& v, const char* key) noexcept
{
  if(auto* m = wireMember(v, key); m && m->IsInt64())
    return m->GetInt64();
  return std::nullopt;
}

//! Whether this is the shape an ObjectPath deserializes from.
//!
//! Handing anything else to JSONObject::Deserializer reads an array that is
//! not one, then a string that is not one, inside the handler that answers a
//! peer -- so it is checked before, not caught after.
inline bool isWireObjectPath(const rapidjson::Value& v) noexcept
{
  if(!v.IsArray())
    return false;

  const auto& strings = score::StringConstant();
  for(const auto& e : v.GetArray())
  {
    const auto* name = wireMember(e, strings.ObjectName.c_str());
    const auto* id = wireMember(e, strings.ObjectId.c_str());
    if(!name || !name->IsString())
      return false;
    if(!id || !id->IsInt())
      return false;
  }
  return true;
}
}
