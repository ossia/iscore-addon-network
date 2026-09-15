#include "IpDialog.hpp"

#include "IpWidget.hpp"

#include <QBoxLayout>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFlags>
#include <QLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QWidget>

#include <array>

namespace Network
{
IpDialog::IpDialog(QWidget* parent)
{
  setLayout(new QVBoxLayout);
  auto widg = new QWidget;
  layout()->addWidget(widg);
  widg->setLayout(new QHBoxLayout);

  m_ipBox = new QLineEdit{"127.0.0.1", this};
  widg->layout()->addWidget(m_ipBox);

  m_portBox = new QSpinBox;
  m_portBox->setMinimum(1);
  m_portBox->setMaximum(65535);
  m_portBox->setValue(9090);
  widg->layout()->addWidget(m_portBox);

  m_terminalBox = new QCheckBox{tr("Remote control only"), this};
  m_terminalBox->setToolTip(
      tr("Edit and watch the score, but do not run it here: no devices are "
         "opened and nothing is played or rendered on this machine."));
  layout()->addWidget(m_terminalBox);

  auto box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  layout()->addWidget(box);

  connect(box, &QDialogButtonBox::accepted, this, &IpDialog::accepted);
  connect(box, &QDialogButtonBox::rejected, this, &IpDialog::rejected);
}

int IpDialog::port() const
{
  return m_port;
}

const QString& IpDialog::ip() const
{
  return m_ip;
}

PeerRole IpDialog::role() const
{
  return m_role;
}

void IpDialog::accepted()
{
  m_ip = m_ipBox->text();

  m_port = m_portBox->value();
  m_role = m_terminalBox->isChecked() ? PeerRole::Terminal : PeerRole::Performer;
  accept();
}

void IpDialog::rejected()
{
  m_port = 0;
  reject();
}
}
