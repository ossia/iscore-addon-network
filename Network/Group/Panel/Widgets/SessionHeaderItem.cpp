#include "SessionHeaderItem.hpp"

#include <Network/Client/Client.hpp>

namespace Network
{
SessionHeaderItem::SessionHeaderItem(const Client& clt)
    : QTableWidgetItem{clt.name()}
    , client{clt.id()}
{
}
}
