#pragma once
#include <QList>
#include <QMap>
#include <QString>
#include <functional>
#include <iscore/tools/std/HashMap.hpp>

namespace std
{
template<>
struct hash<QByteArray>
{
  std::size_t operator()(const QByteArray& id) const
  {
    return qHash(id);
  }
};
}
namespace Network
{
struct NetworkMessage;

class MessageMapper
{
    public:
        void addHandler(QByteArray addr, std::function<void(NetworkMessage)> fun);
        void map(NetworkMessage m);

        bool contains(const QByteArray& b) const;
    private:
        iscore::hash_map<QByteArray, std::function<void(NetworkMessage)>> m_handlers;
};
}
