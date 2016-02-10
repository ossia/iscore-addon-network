#include "NetworkSettings.hpp"
#include "NetworkSettingsModel.hpp"
#include "NetworkSettingsPresenter.hpp"
#include "NetworkSettingsView.hpp"

namespace iscore {
class SettingsPresenter;
}  // namespace iscore

namespace Network
{
NetworkSettings::NetworkSettings()
{
}

iscore::SettingsDelegateViewInterface* NetworkSettings::makeView()
{
    return new NetworkSettingsView(nullptr);
}

iscore::SettingsDelegatePresenterInterface* NetworkSettings::makePresenter_impl(
        iscore::SettingsDelegateModelInterface& m,
        iscore::SettingsDelegateViewInterface& v,
        QObject* parent)
{
    return new NetworkSettingsPresenter(
                static_cast<NetworkSettingsModel&>(m),
                static_cast<NetworkSettingsView&>(v),
                parent);
}

iscore::SettingsDelegateModelInterface* NetworkSettings::makeModel()
{
    return new NetworkSettingsModel();
}
}
