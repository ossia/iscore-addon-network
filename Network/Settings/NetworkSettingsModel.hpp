#pragma once
#include <score/plugins/settingsdelegate/SettingsDelegateModel.hpp>
#include <QString>
#include <score_addon_network_export.h>
namespace Network
{
namespace Settings
{
class SCORE_ADDON_NETWORK_EXPORT Model : public score::SettingsDelegateModel
{
        Q_OBJECT
        Q_PROPERTY(QString ClientName READ getClientName WRITE setClientName NOTIFY ClientNameChanged FINAL)
        Q_PROPERTY(int ClientPort READ getClientPort WRITE setClientPort NOTIFY ClientPortChanged FINAL)
        Q_PROPERTY(int MasterPort READ getMasterPort WRITE setMasterPort NOTIFY MasterPortChanged FINAL)
        Q_PROPERTY(int PlayerPort READ getMasterPort WRITE setMasterPort NOTIFY MasterPortChanged FINAL)
    public:
        Model(QSettings& set, const score::ApplicationContext& ctx);

        SCORE_SETTINGS_PARAMETER_HPP(QString, ClientName)
        SCORE_SETTINGS_PARAMETER_HPP(int, ClientPort)
        SCORE_SETTINGS_PARAMETER_HPP(int, MasterPort)
        SCORE_SETTINGS_PARAMETER_HPP(int, PlayerPort)

    private:
        QString m_ClientName;
        int m_ClientPort{};
        int m_MasterPort{};
        int m_PlayerPort{};

};

SCORE_SETTINGS_PARAMETER(Model, ClientName)
SCORE_SETTINGS_PARAMETER(Model, ClientPort)
SCORE_SETTINGS_PARAMETER(Model, MasterPort)
SCORE_SETTINGS_PARAMETER(Model, PlayerPort)

}
}
