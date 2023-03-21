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
  float c_target_temperature = NAN;  // Initialize to NAN to differentiate between 0 which is a valid value.
  esphome::optional<climate::ClimateFanMode> c_fan_mode;
  float c_current_temperature = NAN;
  climate::ClimateAction c_action;

  const bool operator!=(ClimateState &rhs) const {
    return c_action != rhs.c_action || c_fan_mode != rhs.c_fan_mode || c_mode != rhs.c_mode ||
           // clang-format off
           ((!std::isnan(c_current_temperature) || !std::isnan(rhs.c_current_temperature)) && (c_current_temperature != rhs.c_current_temperature)) ||
           ((!std::isnan(c_target_temperature) || !std::isnan(rhs.c_target_temperature)) && (c_target_temperature != rhs.c_target_temperature));
    // clang-format on
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
  ClimateState lastPublishedState_{};
};

}  // namespace mitsubishi_uart
}  // namespace esphome
