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
}
