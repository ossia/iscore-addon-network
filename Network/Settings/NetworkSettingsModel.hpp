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
        W_OBJECT(Model)
    public:
        Model(QSettings& set, const score::ApplicationContext& ctx);

        SCORE_SETTINGS_PARAMETER_HPP(SCORE_ADDON_NETWORK_EXPORT, QString, ClientName)
        SCORE_SETTINGS_PARAMETER_HPP(SCORE_ADDON_NETWORK_EXPORT, int, ClientPort)
        SCORE_SETTINGS_PARAMETER_HPP(SCORE_ADDON_NETWORK_EXPORT, int, MasterPort)
        SCORE_SETTINGS_PARAMETER_HPP(SCORE_ADDON_NETWORK_EXPORT, int, PlayerPort)

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
