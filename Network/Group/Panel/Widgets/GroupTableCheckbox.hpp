#pragma once
#include <QWidget>
#include <wobjectdefs.h>

class QCheckBox;

namespace Network
{
class GroupTableCheckbox : public QWidget
{
        W_OBJECT(GroupTableCheckbox)
    public:
        GroupTableCheckbox();

        int state();

        void stateChanged(int w) W_SIGNAL(stateChanged, w);

        void setState(int state); W_SLOT(setState)

    private:
        QCheckBox* m_cb;

};
}
