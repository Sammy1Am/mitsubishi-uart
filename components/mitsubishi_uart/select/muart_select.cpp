#include "muart_select.h"

namespace esphome{
namespace mitsubishi_uart{

void MUARTSelect::control(const std::string &value) {
  // TODO, Tell this->parent_ to update the vane direction
  ESP_LOGD(TAG, "Select control called for %s, active is %s", value.c_str(), this->traits.get_options().at(this->active_index().value()).c_str());
  // TODO: figure ^ out how to get TAG here instead of a string.
  this->parent_->set_select_vane_direction(this);
}

}  // namespace mitsubishi_uart
}  // namespace esphome