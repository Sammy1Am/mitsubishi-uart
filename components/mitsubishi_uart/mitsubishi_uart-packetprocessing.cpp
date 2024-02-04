#include "mitsubishi_uart.h"

namespace esphome {
namespace mitsubishi_uart {

// Packet Handlers
void MitsubishiUART::processPacket(const Packet &packet) {
  ESP_LOGI(TAG, "Generic unhandled packet type %x received.", packet.getPacketType());
  ESP_LOGD(TAG, packet.to_string().c_str());
};

void MitsubishiUART::processPacket(const ConnectResponsePacket &packet) {
  ESP_LOGV(TAG, "Processing %s", packet.to_string().c_str());
  // Not sure if there's any needed content in this response, so assume we're connected.
  hpConnected = true;
  ESP_LOGI(TAG, "Heatpump connected.");
};
void MitsubishiUART::processPacket(const ExtendedConnectResponsePacket &packet) {
  ESP_LOGV(TAG, "Processing %s", packet.to_string().c_str());
  // Not sure if there's any needed content in this response, so assume we're connected.
  // TODO: Is there more useful info in these?
  hpConnected = true;
  ESP_LOGI(TAG, "Heatpump connected.");
};

void MitsubishiUART::processPacket(const SettingsGetResponsePacket &packet) {
  ESP_LOGD(TAG, "Processing settings packet...");
  ESP_LOGV(TAG, "Processing %s", packet.to_string().c_str());

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
      fanChanged = set_fan_mode_(climate::CLIMATE_FAN_QUIET);
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

  // TODO: It would probably be nice to have the enum->string mapping defined somewhere to avoid typos/errors
  const std::string old_vane_position = vane_position_select->state;
  switch(packet.getVane()) {
    case SettingsSetRequestPacket::VANE_AUTO:
      vane_position_select->state = "Auto";
      break;
    case SettingsSetRequestPacket::VANE_1:
      vane_position_select->state = "1";
      break;
    case SettingsSetRequestPacket::VANE_2:
      vane_position_select->state = "2";
      break;
    case SettingsSetRequestPacket::VANE_3:
      vane_position_select->state = "3";
      break;
    case SettingsSetRequestPacket::VANE_4:
      vane_position_select->state = "4";
      break;
    case SettingsSetRequestPacket::VANE_5:
      vane_position_select->state = "5";
      break;
    case SettingsSetRequestPacket::VANE_SWING:
      vane_position_select->state = "Swing";
      break;
    default:
      ESP_LOGW(TAG, "Vane in unknown position %x", packet.getVane());
  }
  publishOnUpdate |= (old_vane_position != vane_position_select->state);


  const std::string old_horizontal_vane_position = horizontal_vane_position_select->state;
  switch(packet.getHorizontalVane()) {
    case SettingsSetRequestPacket::HV_LEFT_FULL:
      horizontal_vane_position_select->state = "<<";
      break;
    case SettingsSetRequestPacket::HV_LEFT:
      horizontal_vane_position_select->state = "<";
      break;
    case SettingsSetRequestPacket::HV_CENTER:
      horizontal_vane_position_select->state = "|";
      break;
    case SettingsSetRequestPacket::HV_RIGHT:
      horizontal_vane_position_select->state = ">";
      break;
    case SettingsSetRequestPacket::HV_RIGHT_FULL:
      horizontal_vane_position_select->state = ">>";
      break;
    case SettingsSetRequestPacket::HV_SPLIT:
      horizontal_vane_position_select->state = "<>";
      break;
    case SettingsSetRequestPacket::HV_SWING:
      horizontal_vane_position_select->state = "Swing";
      break;
    default:
      ESP_LOGW(TAG, "Vane in unknown horizontal position %x", packet.getVane());
  }
  publishOnUpdate |= (old_horizontal_vane_position != horizontal_vane_position_select->state);
};

void MitsubishiUART::processPacket(const CurrentTempGetResponsePacket &packet) {
  ESP_LOGV(TAG, "Processing %s", packet.to_string().c_str());
  // This will be the same as the remote temperature if we're using a remote sensor, otherwise the internal temp
  const float old_current_temperature = current_temperature;
  current_temperature = packet.getCurrentTemp();

  if (current_temperature_sensor) {
    current_temperature_sensor->raw_state = current_temperature;
  }

  publishOnUpdate |= (old_current_temperature != current_temperature);
};

void MitsubishiUART::processPacket(const StatusGetResponsePacket &packet) {
  ESP_LOGV(TAG, "Processing %s", packet.to_string().c_str());
  const climate::ClimateAction old_action = action;

  // If mode is off, action is off
  if (mode == climate::CLIMATE_MODE_OFF) {
    action = climate::CLIMATE_ACTION_OFF;
  }
  // If mode is fan only, packet.getOperating() may be false, but the fan is running
  else if (mode == climate::CLIMATE_MODE_FAN_ONLY) {
    action = climate::CLIMATE_ACTION_FAN;
  }
  // If mode is anything other than off or fan, and the unit is operating, determine the action
  else if (packet.getOperating()) {
    switch (mode) {
      case climate::CLIMATE_MODE_HEAT:
        action = climate::CLIMATE_ACTION_HEATING;
        break;
      case climate::CLIMATE_MODE_COOL:
        action = climate::CLIMATE_ACTION_COOLING;
        break;
      case climate::CLIMATE_MODE_DRY:
        action = climate::CLIMATE_ACTION_DRYING;
        break;
      // TODO: This only works if we get an update while the temps are in this configuration
      // Surely there's some info from the heat pump about which of these modes it's in?
      case climate::CLIMATE_MODE_HEAT_COOL:
        if (current_temperature > target_temperature) {
          action = climate::CLIMATE_ACTION_COOLING;
        } else if (current_temperature < target_temperature) {
          action = climate::CLIMATE_ACTION_HEATING;
        }
        // When the heat pump *changes* to a new action, these temperature comparisons should be accurate.
        // If the mode hasn't changed, but the temps are equal, we can assume the same action and make no change.
        // If the unit overshoots, this still doesn't work.
        break;
    }
  }
  // If we're not operating (but not off or in fan mode), we're idle
  // Should be relatively safe to fall through any unknown modes into showing IDLE
  else {
    action = climate::CLIMATE_ACTION_IDLE;
  }

  publishOnUpdate |= (old_action != action);
};
void MitsubishiUART::processPacket(const StandbyGetResponsePacket &packet) {
  ESP_LOGI(TAG, "Unhandled packet StandbyGetResponsePacket received.");
  ESP_LOGD(TAG, packet.to_string().c_str());
};
void MitsubishiUART::processPacket(const RemoteTemperatureSetResponsePacket &packet) {
  ESP_LOGI(TAG, "Unhandled packet RemoteTemperatureSetResponsePacket received.");
  ESP_LOGD(TAG, packet.to_string().c_str());
};

}  // namespace mitsubishi_uart
}  // namespace esphome
