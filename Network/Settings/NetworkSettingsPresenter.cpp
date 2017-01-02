#include <QApplication>
#include <QDebug>
#include <QStyle>

#include "NetworkSettingsModel.hpp"
#include "NetworkSettingsPresenter.hpp"
#include "NetworkSettingsView.hpp"
#include <iscore/command/Command.hpp>
#include <iscore/plugins/settingsdelegate/SettingsDelegatePresenter.hpp>
#include <iscore/tools/Todo.hpp>
namespace iscore {
class SettingsDelegateModel;
class SettingsDelegateView;
class SettingsPresenter;
}  // namespace iscore

namespace Network
{
NetworkSettingsPresenter::NetworkSettingsPresenter(
        NetworkSettingsModel& m,
        NetworkSettingsView& v,
        QObject* parent) :
    SettingsDelegatePresenter {m, v, parent}
{
    auto& net_model = static_cast<NetworkSettingsModel&>(m_model);
    con(net_model, &NetworkSettingsModel::MasterPortChanged,
        this,	   &NetworkSettingsPresenter::updateMasterPort);
    con(net_model, &NetworkSettingsModel::ClientPortChanged,
        this,	   &NetworkSettingsPresenter::updateClientPort);
    con(net_model, &NetworkSettingsModel::ClientNameChanged,
        this,	   &NetworkSettingsPresenter::updateClientName);

    con(v, &NetworkSettingsView::masterPortChanged,
        this, [&] (auto param) {
            m_disp.submitCommand<SetNetworkSettingsModelMasterPort>(this->model(this), param);
    });
    con(v, &NetworkSettingsView::clientPortChanged,
        this, [&] (auto param) {
            m_disp.submitCommand<SetNetworkSettingsModelClientPort>(this->model(this), param);
    });
    con(v, &NetworkSettingsView::clientNameChanged,
        this, [&] (auto param) {
            m_disp.submitCommand<SetNetworkSettingsModelClientName>(this->model(this), param);
    });

    updateMasterPort();
    updateClientPort();
    updateClientName();
}


// Partie modèle -> vue
void NetworkSettingsPresenter::updateMasterPort()
{
    view(this).setMasterPort(model(this).getMasterPort());
}
void NetworkSettingsPresenter::updateClientPort()
{
    view(this).setClientPort(model(this).getClientPort());
}
void NetworkSettingsPresenter::updateClientName()
{
    view(this).setClientName(model(this).getClientName());
}

QIcon NetworkSettingsPresenter::settingsIcon()
{
    return QApplication::style()->standardIcon(QStyle::SP_DriveNetIcon);
}
}
