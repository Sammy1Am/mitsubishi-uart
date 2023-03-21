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
  this->traits_.set_supports_action(true);
  this->traits_.set_supports_current_temperature(true);
  this->traits_.set_supports_two_point_target_temperature(false);
  this->traits_.set_visual_min_temperature(MUART_MIN_TEMP);
  this->traits_.set_visual_max_temperature(MUART_MAX_TEMP);
  this->traits_.set_visual_temperature_step(MUART_TEMPERATURE_STEP);

  /**
   * Climate pushes all its data to Home Assistant immediately when the API connects, this causes
   * the default 0 to be sent as temperatures, but since this is a valid value (0 deg C), it
   * can cause confusion and mess with graphs when looking at the state in HA.  Setting this to
   * NAN gets HA to treat this value as "unavailable" until we have a real value to publish.
   */
  this->target_temperature = NAN;
  this->current_temperature = NAN;
}

climate::ClimateTraits MUARTClimate::traits() { return traits_; }
climate::ClimateTraits &MUARTClimate::config_traits() { return traits_; }

void MUARTClimate::control(const climate::ClimateCall &call) {}

ClimateState MUARTClimate::getCurrentState() {
  ClimateState currentState{};
  currentState.c_action = this->action;
  currentState.c_current_temperature = this->current_temperature;
  currentState.c_fan_mode = this->fan_mode;
  currentState.c_mode = this->mode;
  currentState.c_target_temperature = this->target_temperature;
  return currentState;
}

void MUARTClimate::lazy_publish_state(void *ignored) {
  ClimateState currentState = getCurrentState();
  if (lastPublishedState_ != currentState) {
    this->publish_state();
    lastPublishedState_ = currentState;
  }
}

}  // namespace mitsubishi_uart
}  // namespace esphome
