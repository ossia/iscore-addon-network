#pragma once
#include <iscore/tools/std/Optional.hpp>
#include <iscore/model/Identifier.hpp>
#include <QTableWidget>


namespace Network
{
class Group;

class GroupHeaderItem : public QTableWidgetItem
{
    public:
        explicit GroupHeaderItem(const Group& group);

        const Id<Group> group;
};
}
