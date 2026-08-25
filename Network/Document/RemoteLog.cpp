#include "RemoteLog.hpp"

#include <score/application/ApplicationContext.hpp>
#include <score/document/DocumentContext.hpp>
#include <score/serialization/DataStreamVisitor.hpp>

#include <core/application/ApplicationSettings.hpp>
#include <core/messages/MessagesPanel.hpp>

#include <QObject>
#include <QDataStream>
#include <QTimer>

#include <stdexcept>
#include <mutex>
#include <vector>

#include <Network/Communication/MessageMapper.hpp>
#include <Network/Communication/WireRead.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Session/Session.hpp>

namespace Network
{
namespace
{
class LogBroadcaster;

/**
 * @brief The one place this process's log passes through.
 *
 * The Qt message handler is per *process* and a session is per *document*, so a
 * broadcaster must not install one: hosting a second document would hand the
 * new broadcaster the old handler -- which is this very function -- and the
 * next line logged would recurse until the stack ran out. Installed once here
 * instead, with broadcasters subscribing.
 */
class LogHub
{
public:
  static void subscribe(LogBroadcaster* b);
  static void unsubscribe(LogBroadcaster* b);

private:
  static void handle(QtMsgType, const QMessageLogContext&, const QString&);

  // Guards the list against the handler, which runs on whichever thread
  // logged: decoders, execution and the asio threads all log, and they do so
  // while documents are being torn down.
  static inline std::mutex m_mutex;
  static inline std::vector<LogBroadcaster*> m_subscribers;
  static inline QtMessageHandler m_previous{};
  static inline bool m_installed{};
};

//! Lines this process printed, on their way to the peers of one session.
class LogBroadcaster final : public QObject
{
public:
  LogBroadcaster(Session& session, const score::DocumentContext& ctx, QObject* parent)
      : QObject{parent}
      , m_session{session}
  {
    m_flush.setSingleShot(true);
    m_flush.setInterval(std::max(1, ctx.app.applicationSettings.uiEventRate));
    connect(&m_flush, &QTimer::timeout, this, &LogBroadcaster::flush);

    LogHub::subscribe(this);
  }

  ~LogBroadcaster() override { LogHub::unsubscribe(this); }

  //! Called from the hub, already on this object's thread.
  void queue(const QString& line)
  {
    // A log that outruns the flush would grow without bound and then send one
    // enormous message: past this, the oldest lines are the ones to lose.
    constexpr int max_pending = 2000;
    if(m_pending.size() >= max_pending)
      m_pending.removeFirst();

    m_pending.push_back(line);
    if(!m_flush.isActive())
      m_flush.start();
  }

private:
  void flush()
  {
    // Nobody is watching: the whole point of this machinery is a peer that
    // does not run the score, and a session may have none.
    if(!m_session.hasTerminals())
    {
      m_pending.clear();
      return;
    }

    if(m_pending.isEmpty())
      return;

    auto lines = std::move(m_pending);
    m_pending.clear();

    // Sending can log -- a broken socket says so -- and that line must not come
    // back round as another batch to send.
    m_sending = true;
    m_session.broadcastToTerminals(
        m_session.makeMessage(MessagesAPI::instance().log_lines, lines));
    m_sending = false;
  }

  friend class LogHub;

  Session& m_session;
  QTimer m_flush;
  QStringList m_pending;
  bool m_sending{};
};

void LogHub::subscribe(LogBroadcaster* b)
{
  std::lock_guard lock{m_mutex};
  if(!m_installed)
  {
    // Once, and never removed: taking it back out would restore a handler that
    // may itself have been replaced since.
    m_previous = qInstallMessageHandler(&LogHub::handle);
    m_installed = true;
  }
  m_subscribers.push_back(b);
}

void LogHub::unsubscribe(LogBroadcaster* b)
{
  std::lock_guard lock{m_mutex};
  std::erase(m_subscribers, b);
}

void LogHub::handle(
    QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
  // Chained first, so the console, the crash log and the Messages panel get
  // what they always got even if everything below throws.
  if(m_previous)
    m_previous(type, context, msg);

  static thread_local bool reentered = false;
  if(reentered)
    return;
  reentered = true;

  {
    std::lock_guard lock{m_mutex};
    for(auto* sub : m_subscribers)
    {
      if(sub->m_sending)
        continue;

      // Queued: this is whichever thread logged, and the session is the
      // document's.
      QMetaObject::invokeMethod(
          sub, [sub, msg] { sub->queue(msg); }, Qt::QueuedConnection);
    }
  }

  reentered = false;
}
}

void bindLogBroadcast(
    QObject& owner, Session& session, const score::DocumentContext& ctx)
{
  new LogBroadcaster{session, ctx, &owner};
}

void bindLogDisplay(QObject& owner, Session& session, const score::DocumentContext& ctx)
{
  session.mapper().addHandler(
      &owner, MessagesAPI::instance().log_lines, [&ctx](const NetworkMessage& m) {
    // A plain QDataStream: a list of strings needs no score visitor, and there
    // is none for it. Still inside the guard -- these bytes are a peer's.
    QStringList lines;
    if(!readingWireData("/log/lines", [&] {
         QDataStream stream{m.data};
         stream >> lines;
         if(stream.status() != QDataStream::Ok)
           throw std::runtime_error{"malformed log batch"};
       }))
      return;

    if(auto* plug = ctx.findPlugin<NetworkDocumentPlugin>(); plug && plug->onHostLog)
      plug->onHostLog(lines);

    // Marked: a message about a file that is not there, or a device that would
    // not open, is about the other machine's files and the other machine's
    // hardware.
    if(auto* panel = ctx.app.findPanel<score::MessagesPanelDelegate>())
      for(const auto& line : lines)
        panel->push(QStringLiteral("host| ") + line, QColor{0x9C, 0xB8, 0xD4});
      });
}
}
