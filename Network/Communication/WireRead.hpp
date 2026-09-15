#pragma once
#include <score/serialization/DataStreamVisitor.hpp>

#include <QDebug>

#include <exception>
#include <utility>

namespace Network
{
//! Deserialize wire data, refusing the message rather than the peer: a
//! QDataStream throws on a truncated frame, and these run inside Qt slots.
template <typename F>
bool readingWireData(const char* what, F&& f) noexcept
{
  // Also tells the deserializers these bytes are not ours: a failed delimiter
  // check breaks into the debugger before it throws, which on a developer
  // build means any peer can stop the process with one bad frame.
  struct Untrusted
  {
    bool previous = std::exchange(score::readingUntrustedData(), true);
    ~Untrusted() { score::readingUntrustedData() = previous; }
  } untrusted;

  try
  {
    f();
    return true;
  }
  catch(const std::exception& e)
  {
    qWarning() << "Ignoring a malformed" << what << "message:" << e.what();
  }
  catch(...)
  {
    qWarning() << "Ignoring a malformed" << what << "message";
  }
  return false;
}
}
