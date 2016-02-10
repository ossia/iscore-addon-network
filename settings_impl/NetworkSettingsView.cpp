#include <QGridLayout>
#include <QLabel>

#include "NetworkSettingsPresenter.hpp"
#include "NetworkSettingsView.hpp"
#include <iscore/plugins/settingsdelegate/SettingsDelegateViewInterface.hpp>

class QObject;

namespace Network
{
NetworkSettingsView::NetworkSettingsView(QObject* parent) :
    iscore::SettingsDelegateViewInterface {parent}
{
    auto layout = new QGridLayout(m_widget);
    m_widget->setLayout(layout);

    m_masterPort->setMinimum(1001);
    m_clientPort->setMinimum(1001);
    m_masterPort->setMaximum(65535);
    m_clientPort->setMaximum(65535);

    layout->addWidget(new QLabel{"Master port"}, 0, 0);
    layout->addWidget(m_masterPort, 0, 1);

    layout->addWidget(new QLabel{"Client port"}, 1, 0);
    layout->addWidget(m_clientPort, 1, 1);

    layout->addWidget(new QLabel{"Client Name"}, 2, 0);
    layout->addWidget(m_clientName, 2, 1);

    connect(m_clientName,	&QLineEdit::textChanged,
            this,			&NetworkSettingsView::clientNameChanged);

    // http://stackoverflow.com/questions/16794695/qt5-overloaded-signals-and-slots
    connect(m_masterPort,	SIGNAL(valueChanged(int)),
            this,			SLOT(masterPortChanged(int)));
    connect(m_clientPort,	SIGNAL(valueChanged(int)),
            this,			SLOT(clientPortChanged(int)));
}

void NetworkSettingsView::setClientName(QString text)
{
    if(text != m_clientName->text())
    {
        m_clientName->setText(text);
    }
}
void NetworkSettingsView::setMasterPort(int val)
{
    if(val != m_masterPort->value())
    {
        m_masterPort->setValue(val);
    }
}
void NetworkSettingsView::setClientPort(int val)
{
    if(val != m_clientPort->value())
    {
        m_clientPort->setValue(val);
    }
}

QWidget* NetworkSettingsView::getWidget()
{
    return m_widget;
}

}
