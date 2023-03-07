#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace mitsubishi_uart {

class MitsubishiUART : public climate::Climate, public PollingComponent {
 public:
  /**
   * Create a new MitsubishiUART with the specified esphome::uart::UARTComponent.
   */
  MitsubishiUART(uart::UARTComponent *uart_comp);

  // Set up the component, initializing the HeatPump object.
  void setup() override;

  // ?
  climate::ClimateTraits traits() override;

  // ?
  void control(const climate::ClimateCall &call) override;

  // Called periodically as PollingComponent
  void update() override;

 private:
  uart::UARTComponent *_hp_uart {nullptr};
  climate::ClimateTraits _traits;
};

}  // namespace mitsubishi_uart
}  // namespace esphome