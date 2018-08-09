#pragma once
#include <score/plugins/settingsdelegate/SettingsDelegateView.hpp>
#include <QLineEdit>
#include <QSpinBox>
#include <QString>
#include <QWidget>
#include <wobjectdefs.h>

class QObject;
namespace Network
{
namespace Settings
{
class Presenter;
class View : public score::GlobalSettingsView
{
        W_OBJECT(View)
    public:
        explicit View();

        void setMasterPort(int val);
        void setClientPort(int val);
        void setClientName(QString text);

        QWidget* getWidget() override;

        void clientNameChanged(const QString& v) W_SIGNAL(clientNameChanged, v);
        void masterPortChanged(int v) W_SIGNAL(masterPortChanged, v);
        void clientPortChanged(int v) W_SIGNAL(clientPortChanged, v);

    private:
        QWidget* m_widget {new QWidget};

        QSpinBox* m_masterPort {new QSpinBox{m_widget}};
        QSpinBox* m_clientPort {new QSpinBox{m_widget}};
        QLineEdit* m_clientName {new QLineEdit{m_widget}};
};
}
}
