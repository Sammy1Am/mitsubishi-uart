#include "muart_climate.h"

namespace esphome {
namespace mitsubishi_uart {

////
// Structure Comparison
// Allows for quick state comparisons (this might get unwieldy eventually)
////

////
// MitsubishiUART
////

MUARTClimate::MUARTClimate() {
  this->_traits.set_supports_action(true);
  this->_traits.set_supports_current_temperature(true);
  this->_traits.set_supports_two_point_target_temperature(false);
  this->_traits.set_visual_min_temperature(MUART_MIN_TEMP);
  this->_traits.set_visual_max_temperature(MUART_MAX_TEMP);
  this->_traits.set_visual_temperature_step(MUART_TEMPERATURE_STEP);
}

climate::ClimateTraits MUARTClimate::traits() { return _traits; }
climate::ClimateTraits &MUARTClimate::config_traits() { return _traits; }

void MUARTClimate::control(const climate::ClimateCall &call) {}

}  // namespace mitsubishi_uart
}  // namespace esphome
