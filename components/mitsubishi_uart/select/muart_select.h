#pragma once

#include "esphome/core/log.h"
#include "esphome/components/select/select.h"
#include "../mitsubishi_uart.h"

namespace esphome{
namespace mitsubishi_uart{

/**
 * A brief explainer on ESPHome's Select:
 * A selection is "chosen" via the Select->make_call().something().perform() function.
 * BUT! this does not actually change the state of the Select.  The state itself is changed
 * BY the publish_state() function.
 * 
 * Which is to say, the only way to truely change the state of the Select is by publishing
 * a new state.  Or conversely there is no way to publish state without implictly setting it.
*/

class MUARTSelect : public MUARTComponent<select::Select, const std::string&> {
  public:
    void control(const std::string &value) override;
    void lazy_publish_state(const std::string &value);
  private:
    std::string lastPublishedState_ {};
};

}
}