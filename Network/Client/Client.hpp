#pragma once
#include <score_addon_network_export.h>
#include <score/model/IdentifiedObject.hpp>

#include <Network/Client/PeerRole.hpp>

namespace Network
{
class SCORE_ADDON_NETWORK_EXPORT Client : public IdentifiedObject<Client>
{
  W_OBJECT(Client)
public:
  Client(Id<Client> id, QObject* parent = nullptr)
      : IdentifiedObject<Client>{id, "Client", parent}
  {
  }

  template <typename Deserializer>
  Client(Deserializer&& vis, QObject* parent)
      : IdentifiedObject{vis, parent}
  {
    vis.writeTo(*this);
  }

  QString name() const { return m_name; }

  void setName(QString arg)
  {
    if(m_name == arg)
      return;

    m_name = arg;
    nameChanged(arg);
  }
  W_SLOT(setName)

  void nameChanged(QString arg) W_SIGNAL(nameChanged, arg);

  //! Settled when this peer joins and constant afterwards: it decides what was
  //! built when the document was read, which cannot be revisited.
  PeerRole role() const noexcept { return m_role; }
  void setRole(PeerRole r) noexcept { m_role = r; }

  W_PROPERTY(QString, name READ name WRITE setName NOTIFY nameChanged)
private:
  QString m_name;
  PeerRole m_role{PeerRole::Performer};
};
}

Q_DECLARE_METATYPE(Id<Network::Client>)
W_REGISTER_ARGTYPE(Id<Network::Client>)
