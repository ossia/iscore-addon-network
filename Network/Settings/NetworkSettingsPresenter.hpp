#pragma once
#include <score/plugins/settingsdelegate/SettingsDelegatePresenter.hpp>
#include <QIcon>

#include <QString>

namespace Network
{
class ClientNameChangedCommand;
class ClientPortChangedCommand;
class MasterPortChangedCommand;
namespace Settings
{
class Model;
class View;
class Presenter :
        public score::SettingsDelegatePresenter
{
        Q_OBJECT
    public:
        using model_type = Model;
        using view_type = View;
        Presenter(
                Model& m,
                View& v,
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
}
