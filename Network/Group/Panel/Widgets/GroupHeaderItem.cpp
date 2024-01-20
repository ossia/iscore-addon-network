#include "GroupHeaderItem.hpp"

#include <Network/Group/Group.hpp>

namespace Network
{
GroupHeaderItem::GroupHeaderItem(const Group& grp)
    : QTableWidgetItem{grp.name()}
    , group{grp.id()}
{
}
}
