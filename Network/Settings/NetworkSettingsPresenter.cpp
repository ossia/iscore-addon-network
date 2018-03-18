#include <QApplication>
#include <QDebug>
#include <QStyle>

#include "NetworkSettingsModel.hpp"
#include "NetworkSettingsPresenter.hpp"
#include "NetworkSettingsView.hpp"
#include <score/command/Command.hpp>
#include <score/plugins/settingsdelegate/SettingsDelegatePresenter.hpp>
#include <score/tools/Todo.hpp>


namespace Network
{
namespace Settings
{
Presenter::Presenter(
    Model& m,
    View& v,
    QObject* parent) :
  score::GlobalSettingsPresenter {m, v, parent}
{
  auto& net_model = static_cast<Model&>(m_model);
  con(net_model, &Model::MasterPortChanged,
      this,	   &Presenter::updateMasterPort);
  con(net_model, &Model::ClientPortChanged,
      this,	   &Presenter::updateClientPort);
  con(net_model, &Model::ClientNameChanged,
      this,	   &Presenter::updateClientName);

  con(v, &View::masterPortChanged,
      this, [&] (auto param) {
    m_disp.submitCommand<SetModelMasterPort>(this->model(this), param);
  });
  con(v, &View::clientPortChanged,
      this, [&] (auto param) {
    m_disp.submitCommand<SetModelClientPort>(this->model(this), param);
  });
  con(v, &View::clientNameChanged,
      this, [&] (auto param) {
    m_disp.submitCommand<SetModelClientName>(this->model(this), param);
  });

  updateMasterPort();
  updateClientPort();
  updateClientName();
}


// Partie modèle -> vue
void Presenter::updateMasterPort()
{
  view(this).setMasterPort(model(this).getMasterPort());
}
void Presenter::updateClientPort()
{
  view(this).setClientPort(model(this).getClientPort());
}
void Presenter::updateClientName()
{
  view(this).setClientName(model(this).getClientName());
}

QIcon Presenter::settingsIcon()
{
  return QApplication::style()->standardIcon(QStyle::SP_DriveNetIcon);
}
}
}
