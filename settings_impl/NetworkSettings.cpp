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

iscore::SettingsDelegatePresenterInterface* NetworkSettings::makePresenter(
        iscore::SettingsPresenter* p,
        iscore::SettingsDelegateModelInterface* m,
        iscore::SettingsDelegateViewInterface* v)
{
    auto pres = new NetworkSettingsPresenter(p, m, v);

    v->setPresenter(pres);

    pres->load();
    pres->view()->doConnections();

    return pres;
}

iscore::SettingsDelegateModelInterface* NetworkSettings::makeModel()
{
    return new NetworkSettingsModel();
}
}
