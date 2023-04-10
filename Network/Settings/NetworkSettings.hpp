#pragma once
#include <score/plugins/settingsdelegate/SettingsDelegateFactory.hpp>

#include <Network/Settings/NetworkSettingsModel.hpp>
#include <Network/Settings/NetworkSettingsPresenter.hpp>
#include <Network/Settings/NetworkSettingsView.hpp>

namespace Network
{
namespace Settings
{
SCORE_DECLARE_SETTINGS_FACTORY(
    Factory, Model, Presenter, View, "ae3f6080-4414-417f-b9b2-83d87de934bd")
}
}
