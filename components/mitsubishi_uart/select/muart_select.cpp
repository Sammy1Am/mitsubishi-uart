#include "muart_select.h"

namespace esphome{
namespace mitsubishi_uart{

void MUARTSelect::control(const std::string &value) {
  // TODO, Tell this->parent_ to update the vane direction
  ESP_LOGD("mitsubishi_uart", "Select control called for %s, active is %s", value.c_str(), this->traits.get_options().at(this->active_index().value()).c_str());
  // TODO: figure ^ out how to get TAG here instead of a string.
}

// If receivedIndex is different than our active index, publish it.
void MUARTSelect::updateIndex(int receivedIndex) {
  if (receivedIndex != this->active_index().value()){
    std::string value = this->traits.get_options().at(receivedIndex);
    this->publish_state(value);
  }
}


}
}