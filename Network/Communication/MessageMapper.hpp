#pragma once
#include <score_addon_network_export.h>
#include <score/serialization/DataStreamVisitor.hpp>
#include <score/tools/std/HashMap.hpp>

#include <QDataStream>
#include <QList>
#include <QMap>
#include <QPointer>
#include <QString>

#include <Network/Communication/NetworkMessage.hpp>
#include <tuplet/tuple.hpp>

#include <functional>

namespace Network
{
struct NetworkMessage;

class SCORE_ADDON_NETWORK_EXPORT MessageMapper
{
public:
  /**
   * @brief Register a handler, for as long as `owner` is alive.
   *
   * The mapper belongs to the session, which outlives the policies that install
   * handlers on it -- nothing owns a Session, while a policy dies with its
   * document. A message arriving in between would run a lambda holding
   * references to freed objects, which is what happened when two peers in one
   * process tore down while messages were still in flight.
   */
  void addHandler(
      const QObject* owner, QByteArray addr,
      std::function<void(const NetworkMessage&)> fun);
  template <typename Fun>
  void addHandler_(const QObject* owner, const QByteArray& data, Fun f)
  {
    addHandler(owner, data, [fun = std::move(f)](const NetworkMessage& m) mutable {
      QDataStream ss{m.data};
      DataStreamOutput s{ss};
      [&]<typename... Args>(void (Fun::*)(const NetworkMessage&, Args...) const) {
        tuplet::tuple<Args...> args;
        tuplet::apply(
            [&](auto&&... a) {
          ((s >> a), ...);
          fun(m, a...);
            },
            args);
          }(&Fun::operator());
    });
  }

  void map(const NetworkMessage& m);

  bool contains(const QByteArray& b) const;

private:
  struct Handler
  {
    QPointer<const QObject> owner;
    std::function<void(const NetworkMessage&)> fun;
  };
  score::hash_map<QByteArray, Handler> m_handlers;
};
}
