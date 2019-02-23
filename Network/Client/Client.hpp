#pragma once
#include <score/model/IdentifiedObject.hpp>

namespace Network
{
class Client : public IdentifiedObject<Client>
{
  W_OBJECT(Client)
public:
  Client(Id<Client> id, QObject* parent = nullptr)
      : IdentifiedObject<Client>{id, "Client", parent}
  {
  }

  template <typename Deserializer>
  Client(Deserializer&& vis, QObject* parent) : IdentifiedObject{vis, parent}
  {
    vis.writeTo(*this);
  }

  QString name() const { return m_name; }

  void setName(QString arg)
  {
    if (m_name == arg)
      return;

    m_name = arg;
    nameChanged(arg);
  }
  W_SLOT(setName)

  void nameChanged(QString arg) W_SIGNAL(nameChanged, arg);

  W_PROPERTY(QString, name READ name WRITE setName NOTIFY nameChanged)
private:
  QString m_name;
};
}

Q_DECLARE_METATYPE(Id<Network::Client>)
W_REGISTER_ARGTYPE(Id<Network::Client>)
