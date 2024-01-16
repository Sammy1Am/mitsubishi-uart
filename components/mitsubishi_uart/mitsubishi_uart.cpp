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
const static Packet PACKET_TEMP_REQ = GetRequestPacket(GetCommand::gc_room_temp);
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

// Previously used for receiving and forwarding UART-- depending on buffer-size, this functionality might be
// better suited in update()
// On the otherhand, the thermostat gets grumpy if it has to wait too long, and with "passthru" as an option
// we may not want to be proxying all thermostat calls
void MitsubishiUART::loop() {

}

// Called periodically as PollingComponent (used for UART sending periodically)
// Possibly used for receiving and forwarding from thermostat (if latency isn't too bad / buffers hold up)
void MitsubishiUART::update() {

  // Receieve and process any packets that are waiting just in case?
  hp_bridge.receivePackets();

  if (!hpConnected) {
    hp_bridge.sendAndReceive(PACKET_CONNECT_REQ);
    if (!hpConnected) {
      ESP_LOGW(TAG, "Connecting failed...");
      return;
    }
  }

  // TODO: For now, just check on temp
  hp_bridge.sendAndReceive(PACKET_SETTINGS_REQ);
  // TODO: maybe yield(); in between requests?

  // TODO: Check and do publishing just once here, rather than for each update.
  // TODO: Alternatively, make these just send() methods and do the receving in the loop() method??  Then check and publish just in here
}

// Called to instruct a change of the climate controls
void MitsubishiUART::control(const climate::ClimateCall &call) {
  // TODO: Actually do stuff
  ESP_LOGI(TAG, "Climate call received");
};

// Packet Handlers
void MitsubishiUART::processGenericPacket(const Packet &packet) {
  ESP_LOGI(TAG, "Unhandled packet type %x received.", packet.getPacketType());
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

  target_temperature = packet.getTargetTemp();

  ESP_LOGI(TAG, "Settings packet");
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
