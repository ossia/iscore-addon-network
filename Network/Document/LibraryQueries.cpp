#include "LibraryQueries.hpp"

#include <Process/ProcessFactory.hpp>
#include <Process/ProcessMetadata.hpp>
#include <Process/ProcessList.hpp>

#include <Library/Panel/LibraryPanelDelegate.hpp>
#include <Process/ProcessMimeSerialization.hpp>
#include <Library/ProcessWidget.hpp>
#include <Library/ProcessesItemModel.hpp>

#include <score/application/GUIApplicationContext.hpp>
#include <score/document/DocumentContext.hpp>
#include <score/plugins/InterfaceList.hpp>
#include <score/plugins/panel/PanelDelegate.hpp>
#include <score/serialization/JSONVisitor.hpp>

#include <Network/Communication/Rpc.hpp>
#include <Network/Communication/WireJson.hpp>

#include <ossia/detail/algorithms.hpp>

#include <stdexcept>
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

namespace
{
//! One node of the library, and everything under it.
//!
//! The whole tree, not the factory list: the shape is meaningful -- categories
//! nest, and "Plugins/Faust" is two levels rather than one name with a slash in
//! it -- and most of what a library holds is not a factory at all. ISF shaders,
//! Faust programs and presets are entries the LibraryInterfaces build by
//! scanning files, and they carry the data that says which file, in customData.
//! Sending factories reproduces neither.
void writeNode(JsonWriter& w, const Library::ProcessNode& node)
{
  const auto name = node.prettyName.toUtf8();
  const auto key = score::uuids::toByteArray(node.key.impl());
  const auto custom = node.customData.toUtf8();

  w.StartObject();
  w.Key("name");
  w.String(name.constData(), name.size());
  w.Key("key");
  w.String(key.constData(), key.size());
  if(!custom.isEmpty())
  {
    w.Key("data");
    w.String(custom.constData(), custom.size());
  }

  if(node.childCount() > 0)
  {
    w.Key("children");
    w.StartArray();
    for(const auto& child : node)
      writeNode(w, child);
    w.EndArray();
  }
  w.EndObject();
}

void readNode(
    const rapidjson::Value& v, Library::ProcessNode& parent, bool topLevel)
{
  const auto name = wireString(v, "name");
  if(!name)
    return;

  Library::ProcessData data;
  data.prettyName = *name;

  // Computed here rather than sent: the icons are score's own resources, the
  // same in both builds, and a QIcon does not travel. Only the top level has
  // one, which is what addCategory does when it builds the tree locally.
  if(topLevel)
    data.icon = Process::getCategoryIcon(data.prettyName);
  if(const auto key = wireString(v, "key"))
    data.key = UuidKey<Process::ProcessModel>::fromString(*key);
  if(const auto custom = wireString(v, "data"))
    data.customData = *custom;

  auto& node = Library::addToLibrary(parent, std::move(data));

  if(const auto* children = wireMember(v, "children"); children && children->IsArray())
    for(const auto& child : children->GetArray())
      readNode(child, node, false);
}
}

//! The library of this machine, panel or no panel.
//!
//! A host run with --no-gui has no library panel -- which is the whole point of
//! that mode, and the shape a score box takes -- so reading the panel's model
//! would mean the machines most likely to be hosts are the ones that cannot
//! answer. One is built here instead when there is no panel, and kept: the
//! model watches the library folder and rescans, and building a second one per
//! request would fight the first over that watch.
const Library::ProcessesItemModel&
libraryModel(const score::GUIApplicationContext& ctx)
{
  if(auto* panel = ctx.findPanel<Library::ProcessPanel>())
    return panel->processWidget().processModel();


  static Library::ProcessesItemModel headless{ctx, nullptr};
  return headless;
}

void bindLibraryQueries(RpcChannel& rpc, const score::DocumentContext& ctx)
{
  rpc.bind(library_processes, [&ctx](const rapidjson::Value&) -> QByteArray {
    const auto& root = libraryModel(ctx.app).rootNode();

    rapidjson::StringBuffer buf;
    JsonWriter w{buf};
    w.StartArray();
    for(const auto& child : root)
      writeNode(w, child);
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
    if(!result.IsArray() || result.Empty())
      return;

    auto& model = panel->processWidget().processModel();

    model.beginResetModel();
    {
      auto& root = model.rootNode();

      // A mirror replaces rather than adds: this machine's processes are not
      // the ones that will run, and offering them would be offering something
      // that cannot happen.
      if(mirror)
      {
        root.erase(root.begin(), root.end());
        for(const auto& child : result.GetArray())
          readNode(child, root, true);
      }
      else
      {
        // A performer runs the score itself, so its own library is as real as
        // the peer's; only what it cannot make is worth adding.
        auto& local = ctx.interfaces<Process::ProcessFactoryList>();
        auto it = ossia::find_if(root, [](const Library::ProcessData& n) {
          return n.prettyName == remoteCategory();
        });
        auto& category
            = (it != root.end())
                  ? *it
                  : Library::addToLibrary(
                        root,
                        Library::ProcessData{{{}, remoteCategory(), {}}, QIcon{}});

        for(const auto& child : result.GetArray())
          if(const auto childKey = wireString(child, "key"))
          {
            const auto key = UuidKey<Process::ProcessModel>::fromString(*childKey);
            if(key != UuidKey<Process::ProcessModel>{} && local.get(key))
              continue;
            readNode(child, category, false);
          }
      }
    }
    model.endResetModel();

    qDebug() << (mirror ? "Library mirrored from the other machine."
                        : "Added the other machine's extras to the library.");
      },
      [](const QString& err) {
    qDebug() << "Could not read the other machine's library:" << err;
      });
}
}
