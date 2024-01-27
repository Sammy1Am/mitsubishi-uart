#pragma once

#include "esphome/core/application.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/select/select.h"
#include "esphome/components/sensor/sensor.h"
#include "muart_packet.h"
#include "muart_bridge.h"
#include <map>

namespace esphome {
namespace mitsubishi_uart {

static const char *TAG = "mitsubishi_uart";

const uint8_t MUART_MIN_TEMP = 16;  // Degrees C
const uint8_t MUART_MAX_TEMP = 31;  // Degrees C
const float MUART_TEMPERATURE_STEP = 0.5;

const std::string FAN_MODE_VERYHIGH = "Very High";

const std::string TEMPERATURE_SOURCE_INTERNAL = "Internal";
const uint32_t TEMPERATURE_SOURCE_TIMEOUT_MS = 420000; // (7min) The heatpump will revert on its own in ~10min

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
  void set_current_temperature_sensor(sensor::Sensor *sensor) {current_temperature_sensor = sensor;};

  // Select setters
  void set_temperature_source_select(select::Select *select) {temperature_source_select = select;};

  // Returns true if select was valid (even if not yet successful) to indicate select component
  // should optimistically publish
  bool select_temperature_source(const std::string &state);

  // Used by external sources to report a temperature
  void temperature_source_report(const std::string &temperature_source, const float &v);

  protected:
    void processGenericPacket(const Packet &packet);
    void processConnectResponsePacket(const ConnectResponsePacket &packet);
    void processExtendedConnectResponsePacket(const ExtendedConnectResponsePacket &packet);
    void processSettingsGetResponsePacket(const SettingsGetResponsePacket &packet);
    void processCurrentTempGetResponsePacket(const CurrentTempGetResponsePacket &packet);
    void processStatusGetResponsePacket(const StatusGetResponsePacket &packet);
    void processStandbyGetResponsePacket(const StandbyGetResponsePacket &packet);
    void processRemoteTemperatureSetResponsePacket(const RemoteTemperatureSetResponsePacket &packet);

    void doPublish();

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

    // Preferences
    void save_preferences();
    void restore_preferences();

    ESPPreferenceObject preferences_;

    // Internal sensors
    sensor::Sensor *current_temperature_sensor;

    // Selects
    select::Select *temperature_source_select;
    std::map<std::string, size_t> temp_select_map; // Used to map strings to indexes for preference storage
    std::string currentTemperatureSource = TEMPERATURE_SOURCE_INTERNAL;
    uint32_t lastReceivedTemperature = millis();
};

struct MUARTPreferences {
  optional<size_t> currentTemperatureSourceIndex = nullopt;  // Index of selected value
  //optional<uint32_t> currentTemperatureSourceHash = nullopt; // Hash of selected value (to make sure it hasn't changed since last save)
};

}  // namespace mitsubishi_uart
}  // namespace esphome
