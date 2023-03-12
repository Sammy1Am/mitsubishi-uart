#pragma once

#include "esphome/core/log.h"
#include "esphome/components/sensor/sensor.h"
#include "../mitsubishi_uart.h"

namespace esphome{
namespace mitsubishi_uart{

class MUARTSensor : public MUARTComponent<sensor::Sensor, float> {
  public:
    void lazy_publish_state(float value);
  private:
    float lastPublishedState_ {};
};

}
}