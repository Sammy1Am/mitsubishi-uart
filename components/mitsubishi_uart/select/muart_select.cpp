#include "muart_select.h"

namespace esphome {
namespace mitsubishi_uart {

void MUARTSelect::control(const std::string &value) {
  // TODO, Tell this->parent_ to update the vane direction
  ESP_LOGD(TAG, "Select control called for %s, active is %s", value.c_str(),
           this->traits.get_options().at(this->active_index().value()).c_str());
}

void MUARTSelect::lazy_publish_state(const std::string &value) {
  if (value != lastPublishedState_) {
    this->publish_state(value);
    lastPublishedState_ = value;
  }
}

}  // namespace mitsubishi_uart
}  // namespace esphome
