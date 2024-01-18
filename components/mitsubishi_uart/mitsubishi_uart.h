#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/select/select.h"
#include "esphome/components/sensor/sensor.h"
#include "muart_packet.h"
#include "muart_bridge.h"

namespace esphome {
namespace mitsubishi_uart {

static const char *TAG = "mitsubishi_uart";
static const char *MUART_VERSION = "0.3.0";

const uint8_t MUART_MIN_TEMP = 16;  // Degrees C
const uint8_t MUART_MAX_TEMP = 31;  // Degrees C
const float MUART_TEMPERATURE_STEP = 0.5;

const std::string FAN_MODE_VERYHIGH = "Very High";

class MitsubishiUART : public PollingComponent, public climate::Climate, public PacketProcessor {
 public:
  /**
   * Create a new MitsubishiUART with the specified esphome::uart::UARTComponent.
   */
  MitsubishiUART(uart::UARTComponent *hp_uart_comp);

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

  // Sensor setters
  void set_current_temperature_sensor(sensor::Sensor *sensor) {this->current_temperature_sensor = sensor;};


  protected:
    void processGenericPacket(const Packet &packet);
    void processConnectResponsePacket(const ConnectResponsePacket &packet);
    void processExtendedConnectResponsePacket(const ExtendedConnectResponsePacket &packet);
    void processSettingsGetResponsePacket(const SettingsGetResponsePacket &packet);
    void processCurrentTempGetResponsePacket(const CurrentTempGetResponsePacket &packet);
    void processStatusGetResponsePacket(const StatusGetResponsePacket &packet);
    void processStandbyGetResponsePacket(const StandbyGetResponsePacket &packet);
    void processRemoteTemperatureSetResponsePacket(const RemoteTemperatureSetResponsePacket &packet);

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
    const uart::UARTComponent &hp_uart;
    // UART packet wrapper for heatpump
    MUARTBridge hp_bridge;
    // Are we connected to the heatpump?
    bool hpConnected = false;
    // Should we call publish on the next update?
    bool publishOnUpdate = false;

    // Internal sensors
    sensor::Sensor *current_temperature_sensor;
};

}  // namespace mitsubishi_uart
}  // namespace esphome
