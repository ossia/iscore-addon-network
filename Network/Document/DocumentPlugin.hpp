#pragma once
#include <iscore/plugins/documentdelegate/plugin/DocumentPlugin.hpp>
#include <QObject>
#include <vector>

#include <iscore/serialization/DataStreamVisitor.hpp>
#include <iscore/serialization/JSONVisitor.hpp>
#include <core/document/Document.hpp>
#include <iscore/actions/Action.hpp>
#include <Scenario/Document/TimeNode/TimeNodeModel.hpp>
#include <functional>
class DataStream;
class JSONObject;
class QWidget;
struct VisitorVariant;

namespace std
{
template <typename tag>
struct hash<Path<tag>>
{
  std::size_t operator()(const Path<tag>& path) const
  {
    return std::hash<ObjectPath>{}(path.unsafePath());
  }
};
}
namespace iscore
{
class Document;
}
namespace Network
{
class NetworkDocumentPlugin;
}
UUID_METADATA(
    ,
    iscore::DocumentPluginFactory,
    Network::NetworkDocumentPlugin,
    "58c9e19a-fde3-47d0-a121-35853fec667d")

ISCORE_DECLARE_ACTION(NetworkPlay, "&Play (Network)", Network, QKeySequence::UnknownKey)
namespace Network
{

  class Session;
  class GroupManager;
  class GroupMetadata;
  class EditionPolicy : public QObject
  {
  public:
    using QObject::QObject;
    virtual Session* session() const = 0;
    virtual void play() = 0;
  };

  class NetworkDocumentPlugin final :
      public iscore::SerializableDocumentPlugin
  {
    Q_OBJECT

    ISCORE_SERIALIZE_FRIENDS
    SERIALIZABLE_MODEL_METADATA_IMPL(NetworkDocumentPlugin)
  public:
    NetworkDocumentPlugin(
          const iscore::DocumentContext& ctx,
          EditionPolicy* policy,
          Id<iscore::DocumentPlugin> id,
          QObject* parent);

    // Loading has to be in two steps since the plugin policy is different from the client
    // and server.
    template<typename Impl>
    NetworkDocumentPlugin(
          const iscore::DocumentContext& ctx,
          Impl& vis,
          QObject* parent):
      iscore::SerializableDocumentPlugin{ctx, vis, parent}
    {
      vis.writeTo(*this);
    }

    void setPolicy(EditionPolicy*);

    GroupManager& groupManager() const
    { return *m_groups; }

    EditionPolicy &policy() const
    { return *m_policy; }

    iscore::hash_map<Path<Scenario::TimeNodeModel>, std::function<void()>> triggers;

  signals:
    void sessionChanged();

  private:
    void setupGroupPlugin(GroupMetadata* grp);

    EditionPolicy* m_policy{};
    GroupManager* m_groups{};

  };

  class DocumentPluginFactory :
      public iscore::DocumentPluginFactory
  {
    ISCORE_CONCRETE("58c9e19a-fde3-47d0-a121-35853fec667d")

        public:
      iscore::DocumentPlugin* load(
        const VisitorVariant& var,
        iscore::DocumentContext& doc,
        QObject* parent) override;
  };
}

