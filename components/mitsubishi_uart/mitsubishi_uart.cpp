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
  this->target_temperature = NAN;
  this->current_temperature = NAN;
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
    publish_state();

    // Check sensors and publish if needed.
    // This is a bit of a hack to avoid needing to publish sensor data immediately as packets arrive.
    // Instead, packet data is written directly to `raw_state` (which doesn't update `state`).  If they
    // differ, calling `publish_state` will update `state` so that it won't be published later
    if (current_temperature_sensor && (current_temperature_sensor->raw_state != current_temperature_sensor->state)) {
      ESP_LOGI(TAG, "Current temp differs, do publish");
      current_temperature_sensor->publish_state(current_temperature_sensor->raw_state);
    }

    publishOnUpdate = false;
  }

  // Request an update from the heatpump
  hp_bridge.sendPacket(PACKET_SETTINGS_REQ); // Needs to be done before status packet for mode logic to work
  hp_bridge.sendPacket(PACKET_STANDBY_REQ);
  hp_bridge.sendPacket(PACKET_STATUS_REQ);
  hp_bridge.sendPacket(PACKET_TEMP_REQ);
}

}  // namespace mitsubishi_uart
}  // namespace esphome
