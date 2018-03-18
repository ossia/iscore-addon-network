#pragma once
#include <QWidget>

class QCheckBox;

namespace Network
{
class GroupTableCheckbox : public QWidget
{
        Q_OBJECT
    public:
        GroupTableCheckbox();

        int state();

    Q_SIGNALS:
        void stateChanged(int);

    public Q_SLOTS:
        void setState(int state);

    private:
        QCheckBox* m_cb;

};
}
