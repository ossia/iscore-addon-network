#include <QSettings>
#include <QVariant>

#include "NetworkSettingsModel.hpp"
#include <iscore/plugins/settingsdelegate/SettingsDelegateModel.hpp>

namespace Network
{
namespace Settings
{
namespace Parameters
{
const iscore::sp<ModelClientNameParameter> ClientName{
  QStringLiteral("Network/ClientName"), "i-score"};
const iscore::sp<ModelClientPortParameter> ClientPort{
    QStringLiteral("Network/ClientPort"), 7777};
const iscore::sp<ModelMasterPortParameter> MasterPort{
    QStringLiteral("Network/MasterPort"), 8888};
const iscore::sp<ModelMasterPortParameter> PlayerPort{
    QStringLiteral("Network/PlayerPort"), 0};

static auto list()
{
  return std::tie(ClientName, ClientPort, MasterPort, PlayerPort);
}
}
Model::Model(QSettings& set, const iscore::ApplicationContext& ctx)
{
  iscore::setupDefaultSettings(set, Parameters::list(), *this);
}

ISCORE_SETTINGS_PARAMETER_CPP(QString, Model, ClientName)
ISCORE_SETTINGS_PARAMETER_CPP(int, Model, ClientPort)
ISCORE_SETTINGS_PARAMETER_CPP(int, Model, MasterPort)
ISCORE_SETTINGS_PARAMETER_CPP(int, Model, PlayerPort)
}
}
