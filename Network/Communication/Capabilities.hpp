#pragma once
#include <score_addon_network_export.h>

#include <QByteArrayList>

class QDataStream;
namespace score
{
struct ApplicationContext;
}

namespace Network
{
/**
 * @brief What a peer's build can actually construct.
 *
 * Peers are not the same program. Protocols and processes are registered
 * conditionally inside plug-ins that ship everywhere -- Syphon exists only on
 * macOS, Spout only on Windows, VST and LV2 not at all in the browser -- so two
 * score instances in one session routinely differ in what they can make.
 *
 * Without this the difference is only discovered by hitting it: a command
 * arrives naming a factory we do not have, and the best the receiver can do is
 * stop and say it has diverged. Exchanging this at join means both ends know
 * up front, and can say so while it is still a fact about the session rather
 * than an error in the middle of an edit.
 *
 * Everything is stored sorted so two peers can be compared without building a
 * set first, and as opaque strings because the whole point is naming things
 * this build has no type for.
 */
struct SCORE_ADDON_NETWORK_EXPORT Capabilities
{
  static Capabilities local(const score::ApplicationContext& ctx);

  //! Device::ProtocolFactory uuids.
  QByteArrayList protocols;

  //! Process::ProcessModelFactory uuids.
  QByteArrayList processes;

  //! "group/key": command keys are names rather than uuids.
  QByteArrayList commands;

  //! What `theirs` can construct and we cannot.
  Capabilities lacking(const Capabilities& theirs) const;

  bool isEmpty() const noexcept;

  //! A line fit to show a person, or empty when nothing is missing.
  QString summary() const;
};

SCORE_ADDON_NETWORK_EXPORT QDataStream&
operator<<(QDataStream& s, const Capabilities& c);
SCORE_ADDON_NETWORK_EXPORT QDataStream& operator>>(QDataStream& s, Capabilities& c);
}
