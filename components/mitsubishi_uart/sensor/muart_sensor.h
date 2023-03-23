#pragma once

#include "esphome/core/log.h"
#include "esphome/components/sensor/sensor.h"
#include "../mitsubishi_uart.h"

namespace esphome {
namespace mitsubishi_uart {

class MUARTSensor : public MUARTComponent<sensor::Sensor, float> {
 public:
  void lazy_publish_state(float value);

 private:
  // Since 0 is often a valid state, intialize to NAN so that first lazy publish will show as different
  float lastPublishedState_{NAN};
};

}  // namespace mitsubishi_uart
}  // namespace esphome
