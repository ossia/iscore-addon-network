#include "GroupPanelFactory.hpp"
#include "GroupPanelDelegate.hpp"

namespace Network
{

std::unique_ptr<score::PanelDelegate> PanelDelegateFactory::make(
        const score::GUIApplicationContext& ctx)
{
    return std::make_unique<PanelDelegate>(ctx);
}

}
