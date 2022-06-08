#pragma once
#include <QDialog>
#include <QString>

class QSpinBox;
class QWidget;
class QLineEdit;

namespace Network
{
class IpWidget;
class IpDialog final : public QDialog
{
public:
  explicit IpDialog(QWidget* parent);

  int port() const;
  const QString& ip() const;

private:
  void accepted();
  void rejected();

  QSpinBox* m_portBox{};
  QLineEdit* m_ipBox{};

  int m_port{};
  QString m_ip;
};
}
