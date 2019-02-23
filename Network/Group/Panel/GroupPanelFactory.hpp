#pragma once
#include <score/plugins/panel/PanelDelegateFactory.hpp>
namespace Network
{

class PanelDelegateFactory final : public score::PanelDelegateFactory
{
  SCORE_CONCRETE("5ec8ea88-5cf3-438d-983c-9437691e3817")

  std::unique_ptr<score::PanelDelegate>
  make(const score::GUIApplicationContext& ctx) override;
};
}
