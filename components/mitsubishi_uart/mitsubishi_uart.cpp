#include "mitsubishi_uart.h"

namespace esphome {
namespace mitsubishi_uart {

// TODO: This should probably live somewhere common eventually (maybe in MUARTBridge where it'll inherently know the direction)
static void logPacket(const char *direction, const Packet &packet) {
  ESP_LOGD(TAG, "%s [%02x] %s", direction, packet.getPacketType(),
           format_hex_pretty(&packet.getBytes()[0], packet.getLength()).c_str());
}

////
// MitsubishiUART
////

MitsubishiUART::MitsubishiUART(uart::UARTComponent &hp_uart_comp) : hp_uart{hp_uart_comp}, hp_bridge{MUARTBridge(hp_uart_comp, *this)} {

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

// Previously used for receiving and forwarding UART-- depending on buffer-size, this functionality might be
// better suited in update()
// On the otherhand, the thermostat gets grumpy if it has to wait too long, and with "passthru" as an option
// we may not want to be proxying all thermostat calls
void MitsubishiUART::loop() {

}

// Called periodically as PollingComponent (used for UART sending periodically)
// Possibly used for receiving and forwarding from thermostat (if latency isn't too bad / buffers hold up)
void MitsubishiUART::update() {

  hp_bridge

  if (!hpConnected) {
    // TODO: Send connect packet
  }

}


// Packet Handlers
void MitsubishiUART::processGenericPacket(const Packet &packet) {
  ESP_LOGI(TAG, "Unhandled packet type %x received.", packet.getPacketType());
  logPacket("<-HP", packet);
};
void MitsubishiUART::processConnectResponsePacket(const ConnectResponsePacket &packet) {
  // Not sure if there's any needed content in this response, so as long as it's valid, we're connected.
  if (packet.isChecksumValid()) {
    hpConnected = true;
    ESP_LOGI(TAG, "Heatpump connected.");
  } else {
    ESP_LOGW(TAG, "");
    logPacket("<-HP", packet);
  }
};
void MitsubishiUART::processExtendedConnectResponsePacket(const ExtendedConnectResponsePacket &packet) {
  ESP_LOGI(TAG, "Unhandled packet type %x received.", packet.getPacketType());
  logPacket("<-HP", packet);
};
void MitsubishiUART::processSettingsGetResponsePacket(const SettingsGetResponsePacket &packet) {
  ESP_LOGI(TAG, "Unhandled packet type %x received.", packet.getPacketType());
  logPacket("<-HP", packet);
};
void MitsubishiUART::processRoomTempGetResponsePacket(const RoomTempGetResponsePacket &packet) {
  ESP_LOGI(TAG, "Unhandled packet type %x received.", packet.getPacketType());
  logPacket("<-HP", packet);
};
void MitsubishiUART::processStatusGetResponsePacket(const StatusGetResponsePacket &packet) {
  ESP_LOGI(TAG, "Unhandled packet type %x received.", packet.getPacketType());
  logPacket("<-HP", packet);
};
void MitsubishiUART::processStandbyGetResponsePacket(const StandbyGetResponsePacket &packet) {
  ESP_LOGI(TAG, "Unhandled packet type %x received.", packet.getPacketType());
  logPacket("<-HP", packet);
};
void MitsubishiUART::processRemoteTemperatureSetResponsePacket(const RemoteTemperatureSetResponsePacket &packet) {
  ESP_LOGI(TAG, "Unhandled packet type %x received.", packet.getPacketType());
  logPacket("<-HP", packet);
};

}  // namespace mitsubishi_uart
}  // namespace esphome
