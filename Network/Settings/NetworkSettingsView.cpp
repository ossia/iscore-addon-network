#include "NetworkSettingsView.hpp"

#include "NetworkSettingsPresenter.hpp"

#include <score/plugins/settingsdelegate/SettingsDelegateView.hpp>
#include <score/widgets/FormWidget.hpp>

#include <QFormLayout>
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
  auto widg = new score::FormWidget{tr("Network")};
  m_widget = widg;
  auto layout = widg->layout();

  m_masterPort->setMinimum(1001);
  m_clientPort->setMinimum(1001);
  m_masterPort->setMaximum(65535);
  m_clientPort->setMaximum(65535);

  layout->addRow("Master port", m_masterPort);
  layout->addRow("Client port", m_clientPort);
  layout->addRow("Client name", m_clientName);

  connect(m_clientName, &QLineEdit::textChanged, this, &View::clientNameChanged);
  connect(
      m_masterPort, qOverload<int>(&QSpinBox::valueChanged), this,
      &View::masterPortChanged);
  connect(
      m_clientPort, qOverload<int>(&QSpinBox::valueChanged), this,
      &View::clientPortChanged);
}

void View::setClientName(QString text)
{
  if(text != m_clientName->text())
  {
    m_clientName->setText(text);
  }
}
void View::setMasterPort(int val)
{
  if(val != m_masterPort->value())
  {
    m_masterPort->setValue(val);
  }
}
void View::setClientPort(int val)
{
  if(val != m_clientPort->value())
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
