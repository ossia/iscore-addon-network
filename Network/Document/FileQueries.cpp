#include "FileQueries.hpp"

#include <score/document/DocumentContext.hpp>
#include <score/serialization/JSONVisitor.hpp>
#include <score/tools/Uri.hpp>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>

#include <Network/Communication/Rpc.hpp>

#include <stdexcept>

namespace Network
{
namespace
{
// Larger belongs on a channel of its own: this one carries the edits too.
// Refused rather than truncated, so half a file cannot pass for the file.
constexpr qint64 max_inline_bytes = 8 * 1024 * 1024;

score::Uri requireUri(const rapidjson::Value& params)
{
  if(!params.IsObject() || !params.HasMember("uri") || !params["uri"].IsString())
    throw std::runtime_error{"a uri must be given"};

  const auto uri = score::Uri::parse(QString::fromUtf8(
      params["uri"].GetString(), params["uri"].GetStringLength()));

  // The schemes are the access control: each names a place inside this score.
  switch(uri.scheme)
  {
    case score::UriScheme::Project:
    case score::UriScheme::Library:
    case score::UriScheme::Cache:
      break;
    default:
      throw std::runtime_error{
          "only project, library and cache locations can be addressed"};
  }

  // By spelling as well as by where it lands: a write opens before it lands.
  for(const auto& part : uri.path.split('/'))
  {
    if(part == "..")
      throw std::runtime_error{"a location cannot point outside itself"};
  }

  return uri;
}

QString requireExistingPath(const score::Uri& uri, const score::DocumentContext& ctx)
{
  const auto path = uri.resolve(ctx);
  if(path.isEmpty())
    throw std::runtime_error{
        QObject::tr("%1 does not point anywhere on this machine")
            .arg(uri.toString())
            .toStdString()};
  return path;
}

//! Guards against a resolved path escaping the place its scheme names -- by a
//! symlink, since "..' is already refused before we get here.
QString rootOf(const score::Uri& uri, const score::DocumentContext& ctx)
{
  const auto root = score::Uri{uri.scheme, {}}.resolve(ctx);
  if(root.isEmpty())
    throw std::runtime_error{
        "this machine has no such location: the document may not have been "
        "saved, or the library may not be set up"};

  // The media cache is made on first use, so not existing yet is ordinary and
  // is not the same as a request pointing somewhere it should not.
  QDir{}.mkpath(root);

  const auto canonical = QFileInfo{root}.canonicalFilePath();
  if(canonical.isEmpty())
    throw std::runtime_error{"that location does not exist on this machine"};
  return canonical;
}

//! For a path that does not exist yet: canonicalise the nearest ancestor that
//! does, so a symlink out of the project is caught before it is written to.
void requireAncestryContained(
    const QString& resolved, const score::Uri& uri, const score::DocumentContext& ctx)
{
  const auto root = rootOf(uri, ctx);

  QDir walk{QFileInfo{resolved}.absolutePath()};
  while(!walk.exists() && !walk.isRoot() && walk.cdUp())
    ;

  const auto existing = QFileInfo{walk.absolutePath()}.canonicalFilePath();
  if(existing.isEmpty() || !score::isUnder(existing, root))
    throw std::runtime_error{"that is outside the location it claims to be in"};
}

void requireContained(
    const QString& resolved, const score::Uri& uri, const score::DocumentContext& ctx)
{
  const auto canonical = QFileInfo{resolved}.canonicalFilePath();
  if(canonical.isEmpty() || !score::isUnder(canonical, rootOf(uri, ctx)))
    throw std::runtime_error{"that is outside the location it claims to be in"};
}
}

void bindFileQueries(RpcChannel& rpc, const score::DocumentContext& ctx)
{
  rpc.bind("fs.list", [&ctx](const rapidjson::Value& params) -> QByteArray {
    const auto uri = requireUri(params);
    const auto path = requireExistingPath(uri, ctx);
    requireContained(path, uri, ctx);

    QDir dir{path};
    if(!dir.exists())
      throw std::runtime_error{"no such directory"};

    rapidjson::StringBuffer buf;
    JsonWriter w{buf};
    w.StartArray();
    for(const auto& entry :
        dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries, QDir::Name))
    {
      const auto name = entry.fileName().toUtf8();
      const auto childUri
          = (score::Uri{
                 uri.scheme,
                 uri.path.isEmpty() ? entry.fileName() : uri.path + '/' + entry.fileName()})
                .toString()
                .toUtf8();

      w.StartObject();
      w.Key("name");
      w.String(name.constData(), name.size());
      w.Key("uri");
      w.String(childUri.constData(), childUri.size());
      w.Key("directory");
      w.Bool(entry.isDir());
      w.Key("size");
      w.Int64(entry.isDir() ? 0 : entry.size());
      w.EndObject();
    }
    w.EndArray();
    return QByteArray{buf.GetString(), (int)buf.GetLength()};
  });

  rpc.bind("fs.read", [&ctx](const rapidjson::Value& params) -> QByteArray {
    const auto uri = requireUri(params);
    const auto path = requireExistingPath(uri, ctx);
    requireContained(path, uri, ctx);

    QFile f{path};
    if(!f.exists())
      throw std::runtime_error{"no such file"};
    if(f.size() > max_inline_bytes)
      throw std::runtime_error{"that file is too large to send this way"};
    if(!f.open(QIODevice::ReadOnly))
      throw std::runtime_error{"that file cannot be read"};

    const auto encoded = f.readAll().toBase64();

    rapidjson::StringBuffer buf;
    JsonWriter w{buf};
    w.StartObject();
    w.Key("data");
    w.String(encoded.constData(), encoded.size());
    w.EndObject();
    return QByteArray{buf.GetString(), (int)buf.GetLength()};
  });

  rpc.bind("fs.write", [&ctx](const rapidjson::Value& params) -> QByteArray {
    const auto uri = requireUri(params);

    // The library is the user's own collection, and not something a peer gets
    // to rearrange. Writing belongs in the project or in the media cache.
    if(uri.scheme == score::UriScheme::Library)
      throw std::runtime_error{"the library cannot be written to from here"};

    if(!params.HasMember("data") || !params["data"].IsString())
      throw std::runtime_error{"no data given"};

    const auto data = QByteArray::fromBase64(QByteArray{
        params["data"].GetString(), (int)params["data"].GetStringLength()});
    if(data.size() > max_inline_bytes)
      throw std::runtime_error{"that is too large to send this way"};

    const auto path = uri.resolve(ctx);
    if(path.isEmpty())
      throw std::runtime_error{"that does not point anywhere on this machine"};

    // All checks first: opening for writing truncates.
    requireAncestryContained(path, uri, ctx);

    QDir{}.mkpath(QFileInfo{path}.absolutePath());
    QFile f{path};
    if(!f.open(QIODevice::WriteOnly))
      throw std::runtime_error{"that file cannot be written"};
    if(f.write(data) != data.size())
      throw std::runtime_error{"the file could not be written in full"};
    f.close();

    return QByteArrayLiteral(R"({"written":true})");
  });
}
}
