#pragma once

#include "esphome/components/climate/climate.h"
#include "../mitsubishi_uart.h"

namespace esphome {
namespace mitsubishi_uart {

const uint8_t MUART_MIN_TEMP = 16;  // Degrees C
const uint8_t MUART_MAX_TEMP = 31;  // Degrees C
const float MUART_TEMPERATURE_STEP = 0.5;

struct ClimateState {
  climate::ClimateMode c_mode;
  float c_target_temperature;
  climate::ClimateFanMode c_fan_mode;
  float c_current_temperature;
  climate::ClimateAction c_action;

  const bool operator!=(ClimateState &rhs) const {
    return c_action != rhs.c_action || c_current_temperature != rhs.c_current_temperature ||
           c_fan_mode != rhs.c_fan_mode || c_mode != rhs.c_mode || c_target_temperature != rhs.c_target_temperature;
  }
};

class MUARTClimate : public MUARTComponent<climate::Climate, void *> {
 public:
  /**
   * Create a new MitsubishiUART with the specified esphome::uart::UARTComponent.
   */
  MUARTClimate();

  // ?
  climate::ClimateTraits traits() override;
  climate::ClimateTraits &config_traits();

  // ?
  void control(const climate::ClimateCall &call) override;
  void lazy_publish_state(void *ignored);  // TODO I'm not sure how better to do this with generics

 private:
  climate::ClimateTraits traits_;
  ClimateState getCurrentState();
  ClimateState lastPublishedState_;
};

}  // namespace mitsubishi_uart
}  // namespace esphome
