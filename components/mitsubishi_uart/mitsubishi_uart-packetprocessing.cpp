#include "mitsubishi_uart.h"

namespace esphome {
namespace mitsubishi_uart {

// Packet Handlers
void MitsubishiUART::processGenericPacket(const Packet &packet) {
  ESP_LOGI(TAG, "Generic unhandled packet type %x received.", packet.getPacketType());
  LOGPACKET(packet, "<-HP");
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

  // Fan
  static bool fanChanged = false;
  switch (packet.getFan()) {
    case 0x00:
      fanChanged = set_fan_mode_(climate::CLIMATE_FAN_AUTO);
      break;
    case 0x01:
      fanChanged = set_custom_fan_mode_(FAN_MODE_QUIET);
      break;
    case 0x02:
      fanChanged = set_fan_mode_(climate::CLIMATE_FAN_LOW);
      break;
    case 0x03:
      fanChanged = set_fan_mode_(climate::CLIMATE_FAN_MEDIUM);
      break;
    case 0x05:
      fanChanged = set_fan_mode_(climate::CLIMATE_FAN_HIGH);
      break;
    case 0x06:
      fanChanged = set_custom_fan_mode_(FAN_MODE_VERYHIGH);
      break;
  }

  publishOnUpdate |= fanChanged;

  // TODO:
  // Horizontal Vane
  // Vane
};

void MitsubishiUART::processCurrentTempGetResponsePacket(const CurrentTempGetResponsePacket &packet) {
  // This will be the same as the remote temperature if we're using a remote sensor, otherwise the internal temp
  const float old_current_temperature = current_temperature;
  current_temperature = packet.getCurrentTemp();

  if (current_temperature_sensor) {
    current_temperature_sensor->raw_state = current_temperature;
  }

  publishOnUpdate |= (old_current_temperature != current_temperature);
};

void MitsubishiUART::processStatusGetResponsePacket(const StatusGetResponsePacket &packet) {
  ESP_LOGI(TAG, "Unhandled packet StatusGetResponsePacket received.");
  LOGPACKET(packet, "<-HP");
};
void MitsubishiUART::processStandbyGetResponsePacket(const StandbyGetResponsePacket &packet) {
  ESP_LOGI(TAG, "Unhandled packet StandbyGetResponsePacket received.");
  LOGPACKET(packet, "<-HP");
};
void MitsubishiUART::processRemoteTemperatureSetResponsePacket(const RemoteTemperatureSetResponsePacket &packet) {
  ESP_LOGI(TAG, "Unhandled packet RemoteTemperatureSetResponsePacket received.");
  LOGPACKET(packet, "<-HP");
};

}  // namespace mitsubishi_uart
}  // namespace esphome
