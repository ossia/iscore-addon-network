#include "DistributedScenarioCommandFactory.hpp"
#include <iscore/command/Command.hpp>

namespace Network
{
namespace Command
{
const CommandGroupKey& DistributedScenarioCommandFactoryName() {
    static const CommandGroupKey key{"DistributedScenario"};
    return key;
}
}
}
