#pragma once
#include <iscore/plugins/settingsdelegate/SettingsDelegatePresenterInterface.hpp>
#include <QIcon>

#include <QString>

namespace iscore {
class Command;
class SettingsDelegateModelInterface;
class SettingsDelegateViewInterface;
class SettingsPresenter;
}  // namespace iscore

namespace Network
{
class ClientNameChangedCommand;
class ClientPortChangedCommand;
class MasterPortChangedCommand;
class NetworkSettingsModel;
class NetworkSettingsView;

class NetworkSettingsPresenter :
        public iscore::SettingsDelegatePresenterInterface
{
        Q_OBJECT
    public:
        using model_type = NetworkSettingsModel;
        using view_type = NetworkSettingsView;
        NetworkSettingsPresenter(
                NetworkSettingsModel& m,
                NetworkSettingsView& v,
                QObject* parent);

    private:
        QString settingsName() override
        {
            return tr("Network");
        }

        QIcon settingsIcon() override;

        void updateMasterPort();
        void updateClientPort();
        void updateClientName();
};
}
