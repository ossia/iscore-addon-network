#include "LibraryQueries.hpp"

#include <Process/ProcessFactory.hpp>
#include <Process/ProcessMetadata.hpp>
#include <Process/ProcessList.hpp>

#include <Library/Panel/LibraryPanelDelegate.hpp>
#include <Process/ProcessMimeSerialization.hpp>
#include <Library/ProcessWidget.hpp>
#include <Library/ProcessEntry.hpp>
#include <Library/ProcessesItemModel.hpp>

#include <score/application/GUIApplicationContext.hpp>
#include <score/document/DocumentContext.hpp>
#include <score/plugins/InterfaceList.hpp>
#include <score/plugins/panel/PanelDelegate.hpp>
#include <score/serialization/JSONVisitor.hpp>

#include <Network/Communication/Rpc.hpp>
#include <Network/Communication/WireJson.hpp>

#include <ossia/detail/algorithms.hpp>

#include <optional>
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
//! One node of the library, and everything under it. The tree rather than the
//! factory list: nesting is meaningful, and most entries are scanned files
//! rather than factories.
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

//! One node of the peer's library, as pure data. Nothing is attached here:
//! the model turns a staged forest into tree nodes, so that every insertion
//! carries the signals a view needs.
std::optional<Library::StagedNode>
readNode(const rapidjson::Value& v, bool topLevel)
{
  const auto name = wireString(v, "name");
  if(!name)
    return std::nullopt;

  Library::StagedNode staged;
  auto& data = staged.data;
  data.prettyName = *name;

  // Computed, not sent: a QIcon does not travel, and the resources are shared.
  if(topLevel)
    data.icon = Process::getCategoryIcon(data.prettyName);
  if(const auto key = wireString(v, "key"))
    data.key = UuidKey<Process::ProcessModel>::fromString(*key);
  if(const auto custom = wireString(v, "data"))
    data.customData = *custom;

  if(const auto* children = wireMember(v, "children"); children && children->IsArray())
    for(const auto& child : children->GetArray())
      if(auto c = readNode(child, false))
        staged.children.push_back(std::move(*c));

  return staged;
}
}

//! The library of this machine, panel or no panel: a --no-gui host has none.
//! Kept rather than rebuilt per request, since the model watches the folder.
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

    if(mirror)
    {
      // Replaces rather than adds: this machine's processes will not run.
      std::vector<Library::StagedNode> forest;
      for(const auto& child : result.GetArray())
        if(auto node = readNode(child, true))
          forest.push_back(std::move(*node));
      model.replaceRoot(std::move(forest));
    }
    else
    {
      // A performer runs the score itself, so its own library is as real as
      // the peer's; only what it cannot make is worth adding. It goes under
      // one category at the root: these processes answer to no local factory,
      // so there is no process node to anchor them to.
      auto& local = ctx.interfaces<Process::ProcessFactoryList>();
      for(const auto& child : result.GetArray())
      {
        const auto childKey = wireString(child, "key");
        if(!childKey)
          continue;
        const auto key = UuidKey<Process::ProcessModel>::fromString(*childKey);
        if(key != UuidKey<Process::ProcessModel>{} && local.get(key))
          continue;

        if(auto node = readNode(child, false))
        {
          Library::ProcessEntry e;
          e.atRoot = true;
          e.categoryPath = QStringList{remoteCategory()};
          e.node = std::move(*node);
          model.publish(std::move(e));
        }
      }
      model.flushPending();
    }

    qDebug() << (mirror ? "Library mirrored from the other machine."
                        : "Added the other machine's extras to the library.");
      },
      [](const QString& err) {
    qDebug() << "Could not read the other machine's library:" << err;
      });
}
}
