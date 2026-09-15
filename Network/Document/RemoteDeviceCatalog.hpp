#pragma once
#include <Device/Protocol/DeviceCatalog.hpp>

#include <score/model/Identifier.hpp>

#include <QObject>
#include <QPointer>

#include <score_addon_network_export.h>

namespace Network
{
class Client;
class RpcChannel;

/**
 * @brief The protocols and hardware of the machine the score runs on.
 *
 * A terminal's "add a device" must offer what the score can actually reach.
 * Its own MIDI ports and cameras are not that: the score plays elsewhere and
 * will never see them.
 *
 * The protocol list is asked for once and kept: what a machine is built with
 * does not change while it runs. Enumeration is asked for every time, because
 * what is plugged into it does, and a list from ten minutes ago is worse than
 * none.
 *
 * A QObject so that it can be parented to the document's network plug-in: the
 * document holds a bare pointer to it and must outlive nothing.
 */
class SCORE_ADDON_NETWORK_EXPORT RemoteDeviceCatalog final
    : public QObject
    , public Device::DeviceCatalog
{
public:
  RemoteDeviceCatalog(RpcChannel& rpc, Id<Client> peer, QObject* parent);
  ~RemoteDeviceCatalog() override;

  std::vector<Protocol> protocols() const override;
  void
  enumerate(const UuidKey<Device::ProtocolFactory>& protocol, OnDevice) override;

private:
  QPointer<RpcChannel> m_rpc;
  Id<Client> m_peer;
  std::vector<Protocol> m_protocols;
};
}
