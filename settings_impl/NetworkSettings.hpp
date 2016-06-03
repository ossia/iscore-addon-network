#pragma once
#include <iscore/plugins/settingsdelegate/SettingsDelegateFactory.hpp>

#include <settings_impl/NetworkSettingsModel.hpp>
#include <settings_impl/NetworkSettingsPresenter.hpp>
#include <settings_impl/NetworkSettingsView.hpp>

namespace Network
{
namespace Settings
{
ISCORE_DECLARE_SETTINGS_FACTORY(Factory, Model, Presenter, View, "ae3f6080-4414-417f-b9b2-83d87de934bd")
}
}
