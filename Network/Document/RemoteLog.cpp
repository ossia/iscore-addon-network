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

#include <Network/Communication/MessageMapper.hpp>
#include <Network/Communication/WireRead.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Session/Session.hpp>

namespace Network
{
namespace
{
//! Lines this process printed, on their way to the peers. The Qt handler runs
//! on whichever thread logged, so nothing here touches the session directly.
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

    // Chained, not replaced: the console output, the crash log and the
    // Messages panel are all downstream of whatever was installed before.
    g_instance = this;
    m_previous = qInstallMessageHandler(&LogBroadcaster::handle);
  }

  ~LogBroadcaster() override
  {
    qInstallMessageHandler(m_previous);
    g_instance = nullptr;
  }

private:
  static void
  handle(QtMsgType type, const QMessageLogContext& context, const QString& msg)
  {
    // Reporting a failure to send would log, which would report a failure to
    // send. One line of recursion is one too many.
    static thread_local bool reentered = false;

    auto* self = g_instance;
    if(self && self->m_previous)
      self->m_previous(type, context, msg);

    if(!self || reentered)
      return;

    reentered = true;
    QMetaObject::invokeMethod(
        self, [self, msg] { self->queue(msg); }, Qt::QueuedConnection);
    reentered = false;
  }

  void queue(const QString& line)
  {
    m_pending.push_back(line);
    if(!m_flush.isActive())
      m_flush.start();
  }

  void flush()
  {
    if(m_pending.isEmpty())
      return;

    auto lines = std::move(m_pending);
    m_pending.clear();
    m_session.broadcastToAllClients(
        m_session.makeMessage(MessagesAPI::instance().log_lines, lines));
  }

  static inline LogBroadcaster* g_instance{};

  Session& m_session;
  QtMessageHandler m_previous{};
  QTimer m_flush;
  QStringList m_pending;
};
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
