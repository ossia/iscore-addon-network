#pragma once
#include <score/tools/std/Optional.hpp>
#include <score/model/IdentifiedObject.hpp>
#include <QString>
#include <QVector>

#include <score/model/Identifier.hpp>
#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>

class DataStream;
class JSONObject;
class QObject;


namespace Network
{

class Client;
// Groups : registered in the session
// Permissions ? for now we will just have, for each interval in a score,
// a chosen group.
// There is a "default" group that runs the interval everywhere.
// The groups are part of the document plugin.
// Each client can be in a group (it will execute all the intervals that are part of this group).

class Group : public IdentifiedObject<Group>
{
        Q_OBJECT
        Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

        SCORE_SERIALIZE_FRIENDS
    public:
        Group(QString name, Id<Group> id, QObject* parent);

        template<typename Deserializer>
        Group(Deserializer&& vis, QObject* parent) :
            IdentifiedObject{vis, parent}
        {
            vis.writeTo(*this);
        }


        QString name() const;
        void setName(QString arg);

        void addClient(Id<Client> clt);
        void removeClient(Id<Client> clt);
        bool hasClient(const Id<Client>& clt) const;

        const QVector<Id<Client>>& clients() const
        { return m_executingClients; }

    Q_SIGNALS:
        void nameChanged(QString arg);

        void clientAdded(Id<Client>);
        void clientRemoved(Id<Client>);

    private:
        QString m_name;

        QVector<Id<Client>> m_executingClients;
};
}
