#include "muart_select.h"

namespace esphome{
namespace mitsubishi_uart{

void MUARTSelect::control(const std::string &value) {
  // TODO, Tell this->parent_ to update the vane direction
  ESP_LOGD("mitsubishi_uart", "Select control called for %s, active is %s", value.c_str(), this->traits.get_options().at(this->active_index().value()).c_str());
  // TODO: figure ^ out how to get TAG here instead of a string.
}

// void MUARTSelect::publish_state(const std::string &state) {
//   ESP_LOGI("mitsubishi_uart","Publish Called");
//   if (state != this->traits.get_options().at(this->active_index().value())){ // TODO How safe is calling .value() like this?
//       Select::publish_state(state);
//       ESP_LOGI("mitsubishi_uart","Published");
//     }
// }

// // If receivedIndex is different than our active index, publish it.
// void MUARTSelect::updateIndex(int receivedIndex) {
//   
// }


}
}