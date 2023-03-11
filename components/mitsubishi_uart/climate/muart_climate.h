#pragma once

#include "esphome/components/climate/climate.h"
#include "../mitsubishi_uart.h"

namespace esphome {
namespace mitsubishi_uart {

const uint8_t MUART_MIN_TEMP = 16;  // Degrees C
const uint8_t MUART_MAX_TEMP = 31;  // Degrees C
const float MUART_TEMPERATURE_STEP = 0.5;

class MUARTClimate : public LazyClimate, public Component {
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
  void set_parent(MitsubishiUART *parent) {this->parent_ = parent;}

 private:
  climate::ClimateTraits _traits;
  MitsubishiUART *parent_;
};

}  // namespace mitsubishi_uart
}  // namespace esphome
