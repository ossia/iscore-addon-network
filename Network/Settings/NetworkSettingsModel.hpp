#pragma once
#include <iscore/plugins/settingsdelegate/SettingsDelegateModel.hpp>
#include <QString>

namespace iscore
{
    class SettingsDelegatePresenter;
}
namespace Network
{
namespace Settings
{
class Model : public iscore::SettingsDelegateModel
{
        Q_OBJECT
        Q_PROPERTY(QString ClientName READ getClientName WRITE setClientName NOTIFY ClientNameChanged FINAL)
        Q_PROPERTY(int ClientPort READ getClientPort WRITE setClientPort NOTIFY ClientPortChanged FINAL)
        Q_PROPERTY(int MasterPort READ getMasterPort WRITE setMasterPort NOTIFY MasterPortChanged FINAL)
    public:
        Model(QSettings& set, const iscore::ApplicationContext& ctx);

        ISCORE_SETTINGS_PARAMETER_HPP(QString, ClientName)
        ISCORE_SETTINGS_PARAMETER_HPP(int, ClientPort)
        ISCORE_SETTINGS_PARAMETER_HPP(int, MasterPort)

    private:
        QString m_ClientName;
        int m_ClientPort{};
        int m_MasterPort{};

};

ISCORE_SETTINGS_PARAMETER(Model, ClientName)
ISCORE_SETTINGS_PARAMETER(Model, ClientPort)
ISCORE_SETTINGS_PARAMETER(Model, MasterPort)

}
}
