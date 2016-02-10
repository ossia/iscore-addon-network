#pragma once
#include <iscore/plugins/settingsdelegate/SettingsDelegateFactoryInterface.hpp>

#include <iscore/plugins/settingsdelegate/SettingsDelegateModelInterface.hpp>
#include <iscore/plugins/settingsdelegate/SettingsDelegatePresenterInterface.hpp>
#include <iscore/plugins/settingsdelegate/SettingsDelegateViewInterface.hpp>

namespace iscore {
class SettingsPresenter;
}  // namespace iscore

namespace Network
{
class NetworkSettings : public iscore::SettingsDelegateFactory
{
    public:
        NetworkSettings();
        virtual ~NetworkSettings() = default;

    private:
        iscore::SettingsDelegateViewInterface* makeView() override;
        iscore::SettingsDelegatePresenterInterface* makePresenter_impl(
                iscore::SettingsDelegateModelInterface& m,
                iscore::SettingsDelegateViewInterface& v,
                QObject* parent) override;
        iscore::SettingsDelegateModelInterface* makeModel() override;
};
}
