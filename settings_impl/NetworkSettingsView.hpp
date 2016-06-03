#pragma once
#include <iscore/plugins/settingsdelegate/SettingsDelegateView.hpp>
#include <QLineEdit>
#include <QSpinBox>
#include <QString>
#include <QWidget>

class QObject;
namespace Network
{
class NetworkSettingsPresenter;
class NetworkSettingsView : public iscore::SettingsDelegateView
{
        Q_OBJECT
    public:
        explicit NetworkSettingsView(QObject* parent);

        void setMasterPort(int val);
        void setClientPort(int val);
        void setClientName(QString text);

        QWidget* getWidget() override;

    signals:
        void clientNameChanged(const QString&);
        void masterPortChanged(int);
        void clientPortChanged(int);

    private:
        QWidget* m_widget {new QWidget};

        QSpinBox* m_masterPort {new QSpinBox{m_widget}};
        QSpinBox* m_clientPort {new QSpinBox{m_widget}};
        QLineEdit* m_clientName {new QLineEdit{m_widget}};
};
}
