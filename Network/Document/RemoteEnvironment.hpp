#pragma once
#include <score/model/Identifier.hpp>
#include <score/tools/Environment.hpp>

#include <QPointer>

#include <score_addon_network_export.h>

namespace Network
{
class Client;
class RpcChannel;

/**
 * @brief The files of the score are on another machine.
 *
 * Backed by the fs.* methods of the session's rpc channel, which is why the
 * interface is asynchronous: an answer arrives when the peer sends it, and
 * there is nowhere to wait in between.
 *
 * isLocal() is false and resolve() returns nothing. That is not a shortcoming
 * to work around -- it is the honest answer, and the reason callers have to ask
 * for bytes rather than open paths of their own.
 */
class SCORE_ADDON_NETWORK_EXPORT RemoteEnvironment final : public score::Environment
{
public:
  RemoteEnvironment(RpcChannel& rpc, Id<Client> peer);
  ~RemoteEnvironment() override;

  bool isLocal() const noexcept override { return false; }
  QString resolve(const score::Uri& uri) const override;

  void
  list(const score::Uri& uri, Callback<std::vector<score::DirEntry>> onListed,
       Callback<Failure> onFailed) override;
  void
  read(const score::Uri& uri, Callback<QByteArray> onRead,
       Callback<Failure> onFailed) override;
  void write(
      const score::Uri& uri, QByteArray data, Done onWritten,
      Callback<Failure> onFailed) override;

private:
  //! False, and reports, once the channel it was made with is gone.
  bool stillConnected(const Callback<Failure>& onFailed) const;

  //! Not a reference: setEditPolicy replaces the channel -- hosting from a
  //! document that was joined does exactly that -- and the environment
  //! outlives the one it was made with.
  QPointer<RpcChannel> m_rpc;
  Id<Client> m_peer;
};
}
