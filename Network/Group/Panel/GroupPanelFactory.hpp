#pragma once
#include <iscore/plugins/panel/PanelDelegateFactory.hpp>
namespace Network
{

class PanelDelegateFactory final :
        public iscore::PanelDelegateFactory
{
        ISCORE_CONCRETE("5ec8ea88-5cf3-438d-983c-9437691e3817")

        std::unique_ptr<iscore::PanelDelegate> make(
                const iscore::GUIApplicationContext& ctx) override;
};

}
