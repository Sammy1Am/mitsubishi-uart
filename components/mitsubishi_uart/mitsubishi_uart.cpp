#include "mitsubishi_uart.h"

namespace esphome {
namespace mitsubishi_uart {

static const char *TAG = "mitsubishi_uart.component";

MitsubishiUART::MitsubishiUART(uart::UARTComponent *uart_comp){}

void MitsubishiUART::setup() {

}

climate::ClimateTraits MitsubishiUART::traits() { return _traits; }

void MitsubishiUART::control(const climate::ClimateCall &call) {

}

void MitsubishiUART::update() {
  ESP_LOGI(TAG, "Update!");
}

}  // namespace mitsubishi_uart
}  // namespace esphome