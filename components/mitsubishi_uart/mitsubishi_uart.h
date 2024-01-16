#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/select/select.h"
#include "esphome/components/sensor/sensor.h"
#include "muart_packet.h"

namespace esphome {
namespace mitsubishi_uart {

static const char *TAG = "mitsubishi_uart";
static const char *MUART_VERSION = "0.3.0";

const uint8_t MUART_MIN_TEMP = 16;  // Degrees C
const uint8_t MUART_MAX_TEMP = 31;  // Degrees C
const float MUART_TEMPERATURE_STEP = 0.5;


class MitsubishiUART : public PollingComponent, public climate::Climate, public PacketProcessor {
 public:
  /**
   * Create a new MitsubishiUART with the specified esphome::uart::UARTComponent.
   */
  MitsubishiUART(uart::UARTComponent *hp_uart_comp) {
    /**
     * Climate pushes all its data to Home Assistant immediately when the API connects, this causes
     * the default 0 to be sent as temperatures, but since this is a valid value (0 deg C), it
     * can cause confusion and mess with graphs when looking at the state in HA.  Setting this to
     * NAN gets HA to treat this value as "unavailable" until we have a real value to publish.
     */
    this->target_temperature = NAN;
    this->current_temperature = NAN;
  }

  // Used to restore state of previous MUART-specific settings (like temperature source or pass-thru mode)
  // Most other climate-state is preserved by the heatpump itself and will be retrieved after connection
  void setup() override;

  // Called repeatedly (used for UART receiving/forwarding)
  void loop() override;

  // Called periodically as PollingComponent (used for UART sending periodically)
  void update() override;

  // Returns default traits for MUART
  climate::ClimateTraits traits() override { return climate_traits_; }

  // Returns a reference to traits for MUART to be used during configuration
  // TODO: Maybe replace this with specific functions for the traits needed in configuration (a la the override fuctions)
  climate::ClimateTraits &config_traits() { return climate_traits_; }

  // Called to instruct a change of the climate controls
  void control(const climate::ClimateCall &call) override;

  protected:
    void processConnectResponsePacket(const ConnectResponsePacket packet);

  private:
    // Default climate_traits for MUART
    climate::ClimateTraits climate_traits_ = []() -> climate::ClimateTraits {
      climate::ClimateTraits ct = climate::ClimateTraits();

      ct.set_supports_action(true);
      ct.set_supports_current_temperature(true);
      ct.set_supports_two_point_target_temperature(false);
      ct.set_visual_min_temperature(MUART_MIN_TEMP);
      ct.set_visual_max_temperature(MUART_MAX_TEMP);
      ct.set_visual_temperature_step(MUART_TEMPERATURE_STEP);

      return ct;
    }();

    // UARTComponent connected to heatpump
    uart::UARTComponent *hp_uart;
};

}  // namespace mitsubishi_uart
}  // namespace esphome
