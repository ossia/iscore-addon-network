#pragma once
#include <score/plugins/panel/PanelDelegate.hpp>
namespace Network
{
class GroupManager;
class Session;
class PanelDelegate final : public QObject, public score::PanelDelegate
{
public:
  PanelDelegate(const score::GUIApplicationContext& ctx);

  void networkPluginReady();

private:
  QWidget* widget() override;

  const score::PanelStatus& defaultPanelStatus() const override;

  void on_modelChanged(score::MaybeDocument oldm, score::MaybeDocument newm)
      override;

  void setView(
      const score::DocumentContext& ctx,
      const GroupManager& mgr,
      const Session* session);

  void setEmptyView();

  void on_update();
  void scanPlugins(const score::DocumentContext& ctx);

  QWidget* m_widget{};
  QWidget* m_subWidget{};

  QMetaObject::Connection m_con;
};
}
