#include "DistributedScenarioCommandFactory.hpp"
#include <iscore/command/Command.hpp>

namespace Network
{
namespace Command
{
const CommandParentFactoryKey& DistributedScenarioCommandFactoryName() {
    static const CommandParentFactoryKey key{"DistributedScenario"};
    return key;
}
}
}
