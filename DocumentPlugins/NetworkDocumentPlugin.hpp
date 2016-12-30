#pragma once
#include <iscore/plugins/documentdelegate/plugin/DocumentPlugin.hpp>
#include <QObject>
#include <vector>

#include <iscore/serialization/DataStreamVisitor.hpp>
#include <iscore/serialization/JSONVisitor.hpp>
#include <core/document/Document.hpp>
#include <iscore/actions/Action.hpp>
class DataStream;
class JSONObject;
class QWidget;
struct VisitorVariant;

namespace iscore
{
    class Document;
}

ISCORE_DECLARE_ACTION(NetworkPlay, "&Play (Network)", Network, QKeySequence::UnknownKey)
namespace Network
{

class Session;
class GroupManager;
class GroupMetadata;
class NetworkPolicyInterface : public QObject
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
    public:
        NetworkDocumentPlugin(
                const iscore::DocumentContext& ctx,
                NetworkPolicyInterface* policy,
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

        void setPolicy(NetworkPolicyInterface*);

        GroupManager& groupManager() const
        { return *m_groups; }

        NetworkPolicyInterface* policy() const
        { return m_policy; }

    signals:
        void sessionChanged();

    private:
        // TODO
        /*
        std::vector<iscore::ElementPluginModelType> elementPlugins() const override;

        iscore::ElementPluginModel* makeElementPlugin(
                const QObject* element,
                iscore::ElementPluginModelType,
                QObject* parent) override;

        iscore::ElementPluginModel* loadElementPlugin(
                const QObject* element,
                const VisitorVariant&,
                QObject* parent) override;

        iscore::ElementPluginModel* cloneElementPlugin(
                const QObject* element,
                iscore::ElementPluginModel*,
                QObject* parent) override;

        virtual QWidget *makeElementPluginWidget(
                const iscore::ElementPluginModel*, QWidget* widg) const override;
        */

        void serialize_impl(const VisitorVariant&) const override;
        ConcreteKey concreteKey() const override;

        void setupGroupPlugin(GroupMetadata* grp);

        NetworkPolicyInterface* m_policy{};
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

