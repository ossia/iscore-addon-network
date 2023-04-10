#include "NetworkSettingsModel.hpp"

#include <score/plugins/settingsdelegate/SettingsDelegateModel.hpp>

#include <QSettings>
#include <QVariant>

#include <wobjectimpl.h>
W_OBJECT_IMPL(Network::Settings::Model)
namespace Network
{
namespace Settings
{
namespace Parameters
{

SETTINGS_PARAMETER_IMPL(ClientName){QStringLiteral("Network/ClientName"), "score"};
SETTINGS_PARAMETER_IMPL(ClientPort){QStringLiteral("Network/ClientPort"), 7777};
SETTINGS_PARAMETER_IMPL(MasterPort){QStringLiteral("Network/MasterPort"), 8888};
SETTINGS_PARAMETER_IMPL(PlayerPort){QStringLiteral("Network/PlayerPort"), 0};

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
