#pragma once
#include <score/serialization/DataStreamVisitor.hpp>
#include <score/tools/std/HashMap.hpp>

#include <QDataStream>
#include <QList>
#include <QMap>
#include <QString>

#include <Network/Communication/NetworkMessage.hpp>
#include <tuplet/tuple.hpp>

#include <functional>

namespace Network
{
struct NetworkMessage;

class MessageMapper
{
public:
  void addHandler(QByteArray addr, std::function<void(const NetworkMessage&)> fun);
  template <typename Fun>
  void addHandler_(const QByteArray& data, Fun f)
  {
    addHandler(data, [fun = std::move(f)](const NetworkMessage& m) mutable {
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
  score::hash_map<QByteArray, std::function<void(const NetworkMessage&)>> m_handlers;
};
}
