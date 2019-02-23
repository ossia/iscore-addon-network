#include "NetworkSettingsView.hpp"

#include "NetworkSettingsPresenter.hpp"

#include <score/plugins/settingsdelegate/SettingsDelegateView.hpp>

#include <QGridLayout>
#include <QLabel>

#include <wobjectimpl.h>
W_OBJECT_IMPL(Network::Settings::View)
namespace Network
{

namespace Settings
{
View::View()
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

  connect(
      m_clientName, &QLineEdit::textChanged, this, &View::clientNameChanged);
  connect(
      m_masterPort,
      qOverload<int>(&QSpinBox::valueChanged),
      this,
      &View::masterPortChanged);
  connect(
      m_clientPort,
      qOverload<int>(&QSpinBox::valueChanged),
      this,
      &View::clientPortChanged);
}

void View::setClientName(QString text)
{
  if (text != m_clientName->text())
  {
    m_clientName->setText(text);
  }
}
void View::setMasterPort(int val)
{
  if (val != m_masterPort->value())
  {
    m_masterPort->setValue(val);
  }
}
void View::setClientPort(int val)
{
  if (val != m_clientPort->value())
  {
    m_clientPort->setValue(val);
  }
}

QWidget* View::getWidget()
{
  return m_widget;
}
}
}
