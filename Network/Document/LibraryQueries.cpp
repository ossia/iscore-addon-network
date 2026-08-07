#include "LibraryQueries.hpp"

#include <Process/ProcessFactory.hpp>
#include <Process/ProcessList.hpp>

#include <Library/Panel/LibraryPanelDelegate.hpp>
#include <Library/ProcessWidget.hpp>
#include <Library/ProcessesItemModel.hpp>

#include <score/application/GUIApplicationContext.hpp>
#include <score/document/DocumentContext.hpp>
#include <score/plugins/InterfaceList.hpp>
#include <score/plugins/panel/PanelDelegate.hpp>
#include <score/serialization/JSONVisitor.hpp>

#include <Network/Communication/Rpc.hpp>

#include <ossia/detail/algorithms.hpp>

#include <vector>

namespace Network
{
namespace
{
constexpr auto library_processes = "library.processes";

//! Where processes the local build does not have are put, so that it is clear
//! they come from elsewhere and will not run here.
const QString& remoteCategory()
{
  static const QString c = QObject::tr("On the other machine");
  return c;
}
}

void bindLibraryQueries(RpcChannel& rpc, const score::DocumentContext& ctx)
{
  rpc.bind(library_processes, [&ctx](const rapidjson::Value&) -> QByteArray {
    rapidjson::StringBuffer buf;
    JsonWriter w{buf};
    w.StartArray();

    for(auto& fac : ctx.app.interfaces<Process::ProcessFactoryList>())
    {
      const auto key = score::uuids::toByteArray(fac.concreteKey().impl());
      const auto name = fac.prettyName().toUtf8();
      const auto category = fac.category().toUtf8();

      w.StartObject();
      w.Key("key");
      w.String(key.constData(), key.size());
      w.Key("name");
      w.String(name.constData(), name.size());
      w.Key("category");
      w.String(category.constData(), category.size());
      w.EndObject();
    }

    w.EndArray();
    return QByteArray{buf.GetString(), (int)buf.GetLength()};
  });
}

void importRemoteLibrary(
    RpcChannel& rpc, const score::GUIApplicationContext& ctx, const Id<Client>& peer,
    bool mirror)
{
  auto* panel = ctx.findPanel<Library::ProcessPanel>();
  if(!panel)
    return;

  rpc.call(
      peer, library_processes, QByteArrayLiteral("{}"),
      [&ctx, panel, mirror](const rapidjson::Value& result) {
    if(!result.IsArray())
      return;

    auto& local = ctx.interfaces<Process::ProcessFactoryList>();
    auto& model = panel->processWidget().processModel();

    // Read out first: the model is reset around the change, and a view must not
    // be walking the tree while it is rebuilt.
    struct Entry
    {
      Library::ProcessData data;
      QString category;
    };
    std::vector<Entry> entries;

    for(const auto& e : result.GetArray())
    {
      if(!e.IsObject() || !e.HasMember("key") || !e.HasMember("name"))
        continue;

      const auto key = UuidKey<Process::ProcessModel>::fromString(QString::fromUtf8(
          e["key"].GetString(), e["key"].GetStringLength()));

      // A performer runs the score itself, so its own processes are as real as
      // the peer's and only the extras are worth adding. A terminal runs
      // nothing, so what it has locally is beside the point.
      if(!mirror && local.get(key))
        continue;

      QString category = mirror ? QObject::tr("Other") : remoteCategory();
      if(mirror && e.HasMember("category") && e["category"].GetStringLength() > 0)
        category = QString::fromUtf8(
            e["category"].GetString(), e["category"].GetStringLength());

      entries.push_back(Entry{
          Library::ProcessData{
              {key,
               QString::fromUtf8(e["name"].GetString(), e["name"].GetStringLength()),
               {}},
              QIcon{}},
          std::move(category)});
    }

    if(entries.empty())
      return;

    model.beginResetModel();
    {
      auto& root = model.rootNode();

      // A mirror replaces rather than adds: this machine's processes are not
      // the ones that will run, and offering them would be offering something
      // that cannot happen.
      if(mirror)
        root.erase(root.begin(), root.end());

      for(auto& entry : entries)
      {
        auto it = ossia::find_if(root, [&](const Library::ProcessData& n) {
          return n.prettyName == entry.category;
        });

        auto& category
            = (it != root.end())
                  ? *it
                  : Library::addToLibrary(
                        root,
                        Library::ProcessData{{{}, entry.category, {}}, QIcon{}});

        Library::addToLibrary(category, std::move(entry.data));
      }
    }
    model.endResetModel();

    qDebug() << (mirror ? "Library mirrored from the other machine:"
                        : "Added from the other machine:")
             << entries.size() << "processes.";
      },
      [](const QString& err) {
    qDebug() << "Could not read the other machine's library:" << err;
      });
}
}
