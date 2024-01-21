#include "mitsubishi_uart.h"

namespace esphome {
namespace mitsubishi_uart {

// Static packets for set requests
static const Packet PACKET_CONNECT_REQ = ConnectRequestPacket();
static const Packet PACKET_SETTINGS_REQ = GetRequestPacket(GetCommand::gc_settings);
static const Packet PACKET_TEMP_REQ = GetRequestPacket(GetCommand::gc_current_temp);
static const Packet PACKET_STATUS_REQ = GetRequestPacket(GetCommand::gc_status);
static const Packet PACKET_STANDBY_REQ = GetRequestPacket(GetCommand::gc_standby);

////
// MitsubishiUART
////

MitsubishiUART::MitsubishiUART(uart::UARTComponent *hp_uart_comp) : hp_uart{*hp_uart_comp}, hp_bridge{MUARTBridge(*hp_uart_comp, *this)} {

  /**
   * Climate pushes all its data to Home Assistant immediately when the API connects, this causes
   * the default 0 to be sent as temperatures, but since this is a valid value (0 deg C), it
   * can cause confusion and mess with graphs when looking at the state in HA.  Setting this to
   * NAN gets HA to treat this value as "unavailable" until we have a real value to publish.
   */
  target_temperature = NAN;
  current_temperature = NAN;
}

// Used to restore state of previous MUART-specific settings (like temperature source or pass-thru mode)
// Most other climate-state is preserved by the heatpump itself and will be retrieved after connection
void MitsubishiUART::setup() {

}

/* Used for receiving and acting on incoming packets as soon as they're available.
  Because packet processing happens as part of the receiving process, packet processing
  should not block for very long (e.g. no publishing inside the packet processing)
*/
void MitsubishiUART::loop() {
  // Loop bridge to handle sending and receiving packets
  hp_bridge.loop();

  // TODO: Check timeout value for received external temps and if exceeded:
  // temperature_source_select->publish_state(TEMPERATURE_SOURCE_INTERNAL);
  // Send packet to HP to tell it to use internal temp sensor
}

/* Called periodically as PollingComponent; used to send packets to connect or request updates.

Possible TODO: If we only publish during updates, since data is received during loop, updates will always
be about `update_interval` late from their actual time.  Generally the update interval should be low enough
(default is 5seconds) this won't pose a practical problem.
*/
void MitsubishiUART::update() {

  // TODO: Temporarily wait 5 seconds on startup to help with viewing logs
  if (millis() < 5000) {
    return;
  }

  // If we're not yet connected, send off a connection request (we'll check again next update)
  if (!hpConnected) {
    hp_bridge.sendPacket(PACKET_CONNECT_REQ);
    return;
  }

  // Before requesting additional updates, publish any changes waiting from packets received
  if (publishOnUpdate){
    doPublish();

    publishOnUpdate = false;
  }

  // Request an update from the heatpump
  hp_bridge.sendPacket(PACKET_SETTINGS_REQ); // Needs to be done before status packet for mode logic to work
  hp_bridge.sendPacket(PACKET_STANDBY_REQ);
  hp_bridge.sendPacket(PACKET_STATUS_REQ);
  hp_bridge.sendPacket(PACKET_TEMP_REQ);
}

void MitsubishiUART::doPublish() {
  publish_state();

  // Check sensors and publish if needed.
  // This is a bit of a hack to avoid needing to publish sensor data immediately as packets arrive.
  // Instead, packet data is written directly to `raw_state` (which doesn't update `state`).  If they
  // differ, calling `publish_state` will update `state` so that it won't be published later
  if (current_temperature_sensor && (current_temperature_sensor->raw_state != current_temperature_sensor->state)) {
    ESP_LOGI(TAG, "Current temp differs, do publish");
    current_temperature_sensor->publish_state(current_temperature_sensor->raw_state);
  }
}

bool MitsubishiUART::select_temperature_source(const std::string &state) {
  currentTemperatureSource = state;
  return true;
}

// Called by temperature_source sensors to report values.  Will only take action if the currentTemperatureSource
// matches the incoming source.  Specifically this means that we are not storing any values
// for sensors other than the current source, and selecting a different source won't have any
// effect until that source reports a temperature.
// TODO: ? Maybe store all temperatures (and report on them using internal sensors??) so that selecting a new
// source takes effect immediately?  Only really needed if source sensors are configured with very slow update times.
void MitsubishiUART::temperature_source_report(const std::string &temperature_source, const float &v) {
  ESP_LOGI(TAG, "Received temperature from %s of %f.", temperature_source.c_str(), v);

  if (currentTemperatureSource == temperature_source) {
    // TODO: Set current temperature optimistically
    // TODO: Send off packet to tell HP what current temperature is
    if (temperature_source_select->state != temperature_source) {
      temperature_source_select->publish_state(temperature_source);
    }
  }
}

}  // namespace mitsubishi_uart
}  // namespace esphome
