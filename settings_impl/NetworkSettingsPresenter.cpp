#include <QApplication>
#include <QDebug>
#include <QStyle>

#include "NetworkSettingsModel.hpp"
#include "NetworkSettingsPresenter.hpp"
#include "NetworkSettingsView.hpp"
#include <iscore/command/Command.hpp>
#include <iscore/plugins/settingsdelegate/SettingsDelegatePresenterInterface.hpp>
#include <iscore/tools/Todo.hpp>
namespace iscore {
class SettingsDelegateModelInterface;
class SettingsDelegateViewInterface;
class SettingsPresenter;
}  // namespace iscore

namespace Network
{
NetworkSettingsPresenter::NetworkSettingsPresenter(
        NetworkSettingsModel& m,
        NetworkSettingsView& v,
        QObject* parent) :
    SettingsDelegatePresenterInterface {m, v, parent}
{
    auto& net_model = static_cast<NetworkSettingsModel&>(m_model);
    con(net_model, &NetworkSettingsModel::masterPortChanged,
        this,	   &NetworkSettingsPresenter::updateMasterPort);
    con(net_model, &NetworkSettingsModel::clientPortChanged,
        this,	   &NetworkSettingsPresenter::updateClientPort);
    con(net_model, &NetworkSettingsModel::clientNameChanged,
        this,	   &NetworkSettingsPresenter::updateClientName);

    con(v, &NetworkSettingsView::masterPortChanged,
        this, [&] (auto param) {
            m_disp.submitCommand<SetMasterPort>(this->model(this), param);
    });
    con(v, &NetworkSettingsView::clientPortChanged,
        this, [&] (auto param) {
            m_disp.submitCommand<SetClientPort>(this->model(this), param);
    });
    con(v, &NetworkSettingsView::clientNameChanged,
        this, [&] (auto param) {
            m_disp.submitCommand<SetClientName>(this->model(this), param);
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
