#pragma once
#include <iscore/tools/std/Optional.hpp>
#include <iscore/model/Identifier.hpp>
#include <QTableWidget>


namespace Network
{
class Client;

class SessionHeaderItem : public QTableWidgetItem
{
    public:
        explicit SessionHeaderItem(const Client& client);

        const Id<Client> client;
};
}
