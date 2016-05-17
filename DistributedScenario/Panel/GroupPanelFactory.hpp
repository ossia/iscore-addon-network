#pragma once
#include <iscore/plugins/panel/PanelDelegate.hpp>
namespace Network
{
class GroupManager;
class Session;
class PanelDelegate final :
        public QObject,
        public iscore::PanelDelegate
{
    public:
        PanelDelegate(
                const iscore::ApplicationContext& ctx);

    private:
        QWidget *widget() override;

        const iscore::PanelStatus& defaultPanelStatus() const override;

        void on_modelChanged(
                maybe_document_t oldm,
                maybe_document_t newm) override;


        void setView(const GroupManager* mgr,
                     const Session* session);

        void setEmptyView();

        void on_update();
        void scanPlugins(const iscore::DocumentContext& ctx);

        QWidget* m_widget{};
        QWidget* m_subWidget{};

        QMetaObject::Connection m_con;
};

// MOVEME
class PanelDelegateFactory final :
        public iscore::PanelDelegateFactory
{
        ISCORE_CONCRETE_FACTORY_DECL("5ec8ea88-5cf3-438d-983c-9437691e3817")

        std::unique_ptr<iscore::PanelDelegate> make(
                const iscore::ApplicationContext& ctx) override;
};

}
