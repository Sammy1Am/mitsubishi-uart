#include "muart_sensor.h"

namespace esphome{
namespace mitsubishi_uart{

void MUARTSensor::lazy_publish_state(float value) {
  if (value != lastPublishedState_) {
    this->publish_state(value);
    lastPublishedState_ = value;
  }
}

}  // namespace mitsubishi_uart
}  // namespace esphome