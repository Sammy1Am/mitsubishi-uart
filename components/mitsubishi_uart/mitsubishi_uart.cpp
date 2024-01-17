#include "mitsubishi_uart.h"

namespace esphome {
namespace mitsubishi_uart {

// TODO: This should probably live somewhere common eventually (maybe in MUARTBridge where it'll inherently know the direction)
static void logPacket(const char *direction, const Packet &packet) {
  ESP_LOGD(TAG, "%s [%02x] %s", direction, packet.getPacketType(),
           format_hex_pretty(&packet.getBytes()[0], packet.getLength()).c_str());
}

// TODO: Can we move these to the packet files?
// Static packets for set requests
const static Packet PACKET_CONNECT_REQ = ConnectRequestPacket();
const static Packet PACKET_SETTINGS_REQ = GetRequestPacket(GetCommand::gc_settings);
const static Packet PACKET_TEMP_REQ = GetRequestPacket(GetCommand::gc_current_temp);
const static Packet PACKET_STATUS_REQ = GetRequestPacket(GetCommand::gc_status);
const static Packet PACKET_STANDBY_REQ = GetRequestPacket(GetCommand::gc_standby);

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
    publishOnUpdate = false;
  }

  // Request an update from the heatpump
  hp_bridge.sendPacket(PACKET_SETTINGS_REQ); // Needs to be done before status packet for mode logic to work
  hp_bridge.sendPacket(PACKET_STANDBY_REQ);
  hp_bridge.sendPacket(PACKET_STATUS_REQ);
  hp_bridge.sendPacket(PACKET_TEMP_REQ);
}

// Called to instruct a change of the climate controls
void MitsubishiUART::control(const climate::ClimateCall &call) {
  // TODO: Actually do stuff
  ESP_LOGI(TAG, "Climate call received");
};

// Packet Handlers
void MitsubishiUART::processGenericPacket(const Packet &packet) {
  ESP_LOGI(TAG, "Generic unhandled packet type %x received.", packet.getPacketType());
  logPacket("<-HP", packet);
};

void MitsubishiUART::processConnectResponsePacket(const ConnectResponsePacket &packet) {
  // Not sure if there's any needed content in this response, so assume we're connected.
  hpConnected = true;
  ESP_LOGI(TAG, "Heatpump connected.");
};
void MitsubishiUART::processExtendedConnectResponsePacket(const ExtendedConnectResponsePacket &packet) {
  // Not sure if there's any needed content in this response, so assume we're connected.
  // TODO: Is there more useful info in these?
  hpConnected = true;
  ESP_LOGI(TAG, "Heatpump connected.");
};

void MitsubishiUART::processSettingsGetResponsePacket(const SettingsGetResponsePacket &packet) {
  ESP_LOGD(TAG, "Processing settings packet...");

  // Mode

  const climate::ClimateMode old_mode = mode;
  if (packet.getPower()) {
    switch (packet.getMode()) {
      case 0x01:
        mode = climate::CLIMATE_MODE_HEAT;
        break;
      case 0x02:
        mode = climate::CLIMATE_MODE_DRY;
        break;
      case 0x03:
        mode = climate::CLIMATE_MODE_COOL;
        break;
      case 0x07:
        mode = climate::CLIMATE_MODE_FAN_ONLY;
        break;
      case 0x08:
        mode = climate::CLIMATE_MODE_HEAT_COOL;
        break;
      default:
        mode = climate::CLIMATE_MODE_OFF;
    }
  } else {
    mode = climate::CLIMATE_MODE_OFF;
  }

  publishOnUpdate |= (old_mode != mode);

  // Temperature
  const float old_target_temperature = target_temperature;
  target_temperature = packet.getTargetTemp();
  publishOnUpdate |= (old_target_temperature != target_temperature);
};

void MitsubishiUART::processCurrentTempGetResponsePacket(const CurrentTempGetResponsePacket &packet) {
  // This will be the same as the remote temperature if we're using a remote sensor, otherwise the internal temp
  const float old_current_temperature = current_temperature;
  current_temperature = packet.getCurrentTemp();
  publishOnUpdate |= (old_current_temperature != current_temperature);
};

void MitsubishiUART::processStatusGetResponsePacket(const StatusGetResponsePacket &packet) {
  ESP_LOGI(TAG, "Unhandled packet StatusGetResponsePacket received.");
  logPacket("<-HP", packet);
};
void MitsubishiUART::processStandbyGetResponsePacket(const StandbyGetResponsePacket &packet) {
  ESP_LOGI(TAG, "Unhandled packet StandbyGetResponsePacket received.");
  logPacket("<-HP", packet);
};
void MitsubishiUART::processRemoteTemperatureSetResponsePacket(const RemoteTemperatureSetResponsePacket &packet) {
  ESP_LOGI(TAG, "Unhandled packet RemoteTemperatureSetResponsePacket received.");
  logPacket("<-HP", packet);
};

}  // namespace mitsubishi_uart
}  // namespace esphome
