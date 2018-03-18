#include <QSettings>
#include <QVariant>

#include "NetworkSettingsModel.hpp"
#include <score/plugins/settingsdelegate/SettingsDelegateModel.hpp>

namespace Network
{
namespace Settings
{
namespace Parameters
{
const score::sp<ModelClientNameParameter> ClientName{
  QStringLiteral("Network/ClientName"), "i-score"};
const score::sp<ModelClientPortParameter> ClientPort{
    QStringLiteral("Network/ClientPort"), 7777};
const score::sp<ModelMasterPortParameter> MasterPort{
    QStringLiteral("Network/MasterPort"), 8888};
const score::sp<ModelMasterPortParameter> PlayerPort{
    QStringLiteral("Network/PlayerPort"), 0};

static auto list()
{
  return std::tie(ClientName, ClientPort, MasterPort, PlayerPort);
}
}
Model::Model(QSettings& set, const score::ApplicationContext& ctx)
{
  score::setupDefaultSettings(set, Parameters::list(), *this);
}

SCORE_SETTINGS_PARAMETER_CPP(QString, Model, ClientName)
SCORE_SETTINGS_PARAMETER_CPP(int, Model, ClientPort)
SCORE_SETTINGS_PARAMETER_CPP(int, Model, MasterPort)
SCORE_SETTINGS_PARAMETER_CPP(int, Model, PlayerPort)
}
}
