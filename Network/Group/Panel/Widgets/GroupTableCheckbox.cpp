#include "GroupTableCheckbox.hpp"

#include <QBoxLayout>
#include <QCheckBox>
#include <qnamespace.h>

#include <wobjectimpl.h>
W_OBJECT_IMPL(Network::GroupTableCheckbox)
namespace Network
{
GroupTableCheckbox::GroupTableCheckbox()
{
  m_cb = new QCheckBox;
  connect(m_cb, &QCheckBox::stateChanged, this, &GroupTableCheckbox::stateChanged);

  auto lay = new QHBoxLayout{this};
  lay->addWidget(m_cb);
  lay->setAlignment(Qt::AlignCenter);
  lay->setContentsMargins(0, 0, 0, 0);
}

int GroupTableCheckbox::state()
{
  return m_cb->checkState();
}

void GroupTableCheckbox::setState(int state)
{
  if(state != m_cb->checkState())
  {
    m_cb->setCheckState(Qt::CheckState(state));
  }
}
}
