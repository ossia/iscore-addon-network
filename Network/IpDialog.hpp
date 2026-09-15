#pragma once
#include <QDialog>
#include <QString>

#include <Network/Client/PeerRole.hpp>

class QCheckBox;
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
  PeerRole role() const;

private:
  void accepted();
  void rejected();

  QSpinBox* m_portBox{};
  QLineEdit* m_ipBox{};
  QCheckBox* m_terminalBox{};

  int m_port{};
  QString m_ip;
  PeerRole m_role{PeerRole::Performer};
};
}
