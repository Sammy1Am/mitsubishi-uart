#include "mitsubishi_uart.h"

namespace esphome {
namespace mitsubishi_uart {

// Called to instruct a change of the climate controls
void MitsubishiUART::control(const climate::ClimateCall &call) {
  // TODO: Actually do stuff
  ESP_LOGI(TAG, "Climate call received");
};

}  // namespace mitsubishi_uart
}  // namespace esphome
