#include "muart_select.h"

namespace esphome{
namespace mitsubishi_uart{

void MUARTSelect::control(const std::string &value) {
  //this->make_call().set_option(value).perform();
  ESP_LOGI(TAG, "Control called %s", value&);
}

}
}