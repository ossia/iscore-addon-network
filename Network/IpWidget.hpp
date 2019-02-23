#pragma once
#include <QFrame>

#include <wobjectdefs.h>

#include <array>

class QEvent;
class QLineEdit;
class QObject;
class QWidget;

namespace Network
{
// Found on stackoverflow :
// http://stackoverflow.com/questions/9306335/an-ip-address-widget-for-qt-similar-to-mfcs-ip-address-control
class IpWidget : public QFrame
{
  W_OBJECT(IpWidget)

  enum
  {
    QTUTL_IP_SIZE = 4,
    MAX_DIGITS = 3
  };

public:
  explicit IpWidget(QWidget* parent = 0);
  ~IpWidget();

  bool eventFilter(QObject* obj, QEvent* event) override;

  std::array<QLineEdit*, QTUTL_IP_SIZE> lineEdits;

  void slotTextChanged(QLineEdit* pEdit);
  W_SLOT(slotTextChanged)

  void signalTextChanged(QLineEdit* pEdit) W_SIGNAL(signalTextChanged, pEdit);

private:
  void MoveNextLineEdit(int i);
  void MovePrevLineEdit(int i);
};
}
W_REGISTER_ARGTYPE(QLineEdit*)
