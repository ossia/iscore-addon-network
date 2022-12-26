#include "NetworkSettingsPresenter.hpp"

#include "NetworkSettingsModel.hpp"
#include "NetworkSettingsView.hpp"

#include <score/command/Command.hpp>
#include <score/plugins/settingsdelegate/SettingsDelegatePresenter.hpp>
#include <score/widgets/SetIcons.hpp>

#include <QApplication>
#include <QDebug>
#include <QStyle>

namespace Network
{
namespace Settings // clang-format off
{
Presenter::Presenter(Model& m, View& v, QObject* parent)
    : score::GlobalSettingsPresenter{m, v, parent}
{
  auto& net_model = static_cast<Model&>(m_model);
  con(net_model, &Model::MasterPortChanged, this, &Presenter::updateMasterPort);
  con(net_model, &Model::ClientPortChanged, this, &Presenter::updateClientPort);
  con(net_model, &Model::ClientNameChanged, this, &Presenter::updateClientName);

  con(v, &View::masterPortChanged, this, [&](auto param) {
    if(param != this->model(this).getMasterPort()) m_disp.submit<SetModelMasterPort>(this->model(this), param);
  });
  con(v, &View::clientPortChanged, this, [&](auto param) {
    if(param != this->model(this).getClientPort()) m_disp.submit<SetModelClientPort>(this->model(this), param);
  });
  con(v, &View::clientNameChanged, this, [&](auto param) {
    if(param != this->model(this).getClientName()) m_disp.submit<SetModelClientName>(this->model(this), param);
  });

  updateMasterPort();
  updateClientPort();
  updateClientName();
}

Presenter::~Presenter() {
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
  return makeIcons(
      QStringLiteral(":/icons/settings_sync_on.png"),
      QStringLiteral(":/icons/settings_sync_off.png"),
      QStringLiteral(":/icons/settings_sync_off.png"));
}
}
}
