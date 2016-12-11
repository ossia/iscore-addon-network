#include "GroupPanelFactory.hpp"
#include "GroupPanelDelegate.hpp"

namespace Network
{

std::unique_ptr<iscore::PanelDelegate> PanelDelegateFactory::make(
        const iscore::GUIApplicationContext& ctx)
{
    return std::make_unique<PanelDelegate>(ctx);
}

}
