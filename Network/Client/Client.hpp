#pragma once
#include <score/model/IdentifiedObject.hpp>

#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>

namespace Network
{
class Client : public IdentifiedObject<Client>
{
        Q_OBJECT
        Q_PROPERTY(QString name
                   READ name
                   WRITE setName
                   NOTIFY nameChanged)
    public:
        Client(Id<Client> id, QObject* parent = nullptr):
            IdentifiedObject<Client>{id, "Client", parent}
        {

        }

        template<typename Deserializer>
        Client(Deserializer&& vis, QObject* parent) :
            IdentifiedObject{vis, parent}
        {
            vis.writeTo(*this);
        }

        QString name() const
        {
            return m_name;
        }

    public Q_SLOTS:
        void setName(QString arg)
        {
            if (m_name == arg)
                return;

            m_name = arg;
            nameChanged(arg);
        }

    Q_SIGNALS:
        void nameChanged(QString arg);

    private:
        QString m_name;
};
}

Q_DECLARE_METATYPE(Id<Network::Client>)
