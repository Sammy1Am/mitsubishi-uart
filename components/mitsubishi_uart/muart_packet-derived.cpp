#include "muart_packet.h"

namespace esphome {
namespace mitsubishi_uart {

// Packet to_strings()

std::string ConnectRequestPacket::to_string() const {
  return "Connect Request: " + Packet::to_string();
}
std::string ConnectResponsePacket::to_string() const {
  return "Connect Response: " + Packet::to_string();
}
std::string ExtendedConnectResponsePacket::to_string() const {
  return "Extended Connect Response: " + Packet::to_string() + CONSOLE_COLOR_PURPLE +
           "\n HeatDisabled:" + (isHeatDisabled() ? "Yes" : "No") +
           " SupportsVane:" + (supportsVane() ? "Yes" : "No") +
           " SupportsVaneSwing:" + (supportsVaneSwing() ? "Yes" : "No") +
           " DryDisabled:" + (isDryDisabled() ? "Yes" : "No") +
           " FanDisabled:" + (isFanDisabled() ? "Yes" : "No") +
           " ExtTempRange:" + (hasExtendedTemperatureRange() ? "Yes" : "No") +
           " AutoFanDisabled:" + (autoFanSpeedDisabled() ? "Yes" : "No") +
           " InstallerSettings:" + (supportsInstallerSettings() ? "Yes" : "No") +
           " TestMode:" + (supportsTestMode() ? "Yes" : "No") +
           " DryTemp:" + (supportsDryTemperature() ? "Yes" : "No") +
           " StatusDisplay:" + (hasStatusDisplay() ? "Yes" : "No") +
           "\n CoolDrySetpoint:" + std::to_string(getMinCoolDrySetpoint()) + "/" + std::to_string(getMaxCoolDrySetpoint()) +
           " HeatSetpoint:" + std::to_string(getMinHeatingSetpoint()) + "/" + std::to_string(getMaxHeatingSetpoint()) +
           " AutoSetpoint:" + std::to_string(getMinAutoSetpoint()) + "/" + std::to_string(getMaxAutoSetpoint()) +
           " FanSpeeds:" + std::to_string(getSupportedFanSpeeds());
}
std::string CurrentTempGetResponsePacket::to_string() const {
  return "Current Temp Response: " + Packet::to_string() + CONSOLE_COLOR_PURPLE +
          "\n Temp:" + std::to_string(getCurrentTemp());
}
std::string SettingsGetResponsePacket::to_string() const {
  return "Settings Response: " + Packet::to_string() + CONSOLE_COLOR_PURPLE + "\n Fan:" + format_hex(getFan()) +
          " Mode:" + format_hex(getMode()) +
          " Power:" + (getPower() == 3  ? "Test" : getPower() > 0 ? "On" : "Off") +
          " TargetTemp:" + std::to_string(getTargetTemp()) +
          " Vane:" + format_hex(getVane()) +
          " HVane:" + format_hex(getHorizontalVane()) +

          "\n PowerLock:" + (lockedPower() ? "Yes" : "No") +
          " ModeLock:" + (lockedMode() ? "Yes" : "No") +
          " TempLock:" + (lockedTemp() ? "Yes" : "No");
}
std::string StandbyGetResponsePacket::to_string() const {
  return "Standby Response: " + Packet::to_string() + CONSOLE_COLOR_PURPLE +
          "\n ServiceFilter:" + (serviceFilter() ? "Yes" : "No") +
          " Defrost:" + (inDefrost() ? "Yes" : "No") +
          " HotAdjust:" + (inHotAdjust() ? "Yes" : "No") +
          " Standby:" + (inStandby() ? "Yes" : "No") +
          " ActualFan:" + std::to_string(getActualFanSpeed()) +
          " AutoMode:" + format_hex(getAutoMode());
}
std::string StatusGetResponsePacket::to_string() const {
  return "Status Response: " + Packet::to_string() + CONSOLE_COLOR_PURPLE +
          "\n CompressorFrequency: " + std::to_string(getCompressorFrequency()) +
          " Operating: " + (getOperating() ? "Yes" : "No");
}
std::string ErrorStateGetResponsePacket::to_string() const {
  return ("Error State Response: " + Packet::to_string() + CONSOLE_COLOR_PURPLE +
          "\n ErrorCode: " + format_hex(getErrorCode()) +
          " ShortCode: " + getShortCode() + "(" + format_hex(getRawShortCode()) + ")");
}
std::string RemoteTemperatureSetRequestPacket::to_string() const {
  return ("Remote Temp Set Request: " + Packet::to_string() + CONSOLE_COLOR_PURPLE +
          "\n Temp:" + std::to_string(getRemoteTemperature()));
}

// TODO: Are there function implementations for packets in the .h file? (Yes)  Should they be here?

// SettingsSetRequestPacket functions

void SettingsSetRequestPacket::addSettingsFlag(const SETTING_FLAG flagToAdd) { addFlag(flagToAdd); }

void SettingsSetRequestPacket::addSettingsFlag2(const SETTING_FLAG2 flag2ToAdd) { addFlag2(flag2ToAdd); }

SettingsSetRequestPacket &SettingsSetRequestPacket::setPower(const bool isOn) {
  pkt_.setPayloadByte(PLINDEX_POWER, isOn ? 0x01 : 0x00);
  addSettingsFlag(SF_POWER);
  return *this;
}

SettingsSetRequestPacket &SettingsSetRequestPacket::setMode(const MODE_BYTE mode) {
  pkt_.setPayloadByte(PLINDEX_MODE, mode);
  addSettingsFlag(SF_MODE);
  return *this;
}

SettingsSetRequestPacket &SettingsSetRequestPacket::setTargetTemperature(const float temperatureDegressC) {
  if (temperatureDegressC < 63.5 && temperatureDegressC > -64.0) {
    pkt_.setPayloadByte(PLINDEX_TARGET_TEMPERATURE, round(temperatureDegressC * 2) + 128);
    addSettingsFlag(SF_TARGET_TEMPERATURE);
  } else {
    ESP_LOGW(PTAG, "Target temp %f is outside valid range.", temperatureDegressC);
  }
  return *this;
}
SettingsSetRequestPacket &SettingsSetRequestPacket::setFan(const FAN_BYTE fan) {
  pkt_.setPayloadByte(PLINDEX_FAN, fan);
  addSettingsFlag(SF_FAN);
  return *this;
}

SettingsSetRequestPacket &SettingsSetRequestPacket::setVane(const VANE_BYTE vane) {
  pkt_.setPayloadByte(PLINDEX_VANE, vane);
  addSettingsFlag(SF_VANE);
  return *this;
}

SettingsSetRequestPacket &SettingsSetRequestPacket::setHorizontalVane(const HORIZONTAL_VANE_BYTE horizontal_vane) {
  pkt_.setPayloadByte(PLINDEX_HORIZONTAL_VANE, horizontal_vane);
  addSettingsFlag2(SF2_HORIZONTAL_VANE);
  return *this;
}

// RemoteTemperatureSetRequestPacket functions

RemoteTemperatureSetRequestPacket &RemoteTemperatureSetRequestPacket::setRemoteTemperature(float temperatureDegressC) {
  if (temperatureDegressC < 63.5 && temperatureDegressC > -64.0) {
    pkt_.setPayloadByte(PLINDEX_REMOTE_TEMPERATURE, round(temperatureDegressC * 2) + 128);
    setFlags(0x01);  // Set flags to say we're providing the temperature
  } else {
    ESP_LOGW(PTAG, "Remote temp %f is outside valid range.", temperatureDegressC);
  }
  return *this;
}
RemoteTemperatureSetRequestPacket &RemoteTemperatureSetRequestPacket::useInternalTemperature() {
  setFlags(0x00);  // Set flags to say to use internal temperature
  return *this;
}

// ExtendedConnectResponsePacket functions
uint8_t ExtendedConnectResponsePacket::getSupportedFanSpeeds() const {
  uint8_t raw_value = ((pkt_.getPayloadByte(7) & 0x10) >> 2) + ((pkt_.getPayloadByte(8) & 0x08) >> 2) +
                      ((pkt_.getPayloadByte(9) & 0x02) >> 1);

  switch (raw_value) {
    case 1:
    case 2:
    case 4:
      return raw_value;
    case 0:
      return 3;
    case 6:
      return 5;

    default:
      ESP_LOGW(PACKETS_TAG, "Unexpected supported fan speeds: %i", raw_value);
      return 0;  // TODO: Depending on how this is used, it might be more useful to just return 3 and hope for the best
  }
}

climate::ClimateTraits ExtendedConnectResponsePacket::asTraits() const {
  auto ct = climate::ClimateTraits();

  // always enabled
  ct.add_supported_mode(climate::CLIMATE_MODE_COOL);
  ct.add_supported_mode(climate::CLIMATE_MODE_OFF);

  if (!this->isHeatDisabled())
    ct.add_supported_mode(climate::CLIMATE_MODE_HEAT);
  if (!this->isDryDisabled())
    ct.add_supported_mode(climate::CLIMATE_MODE_DRY);
  if (!this->isFanDisabled())
    ct.add_supported_mode(climate::CLIMATE_MODE_FAN_ONLY);

  if (this->supportsVaneSwing() && this->supportsHVaneSwing())
    ct.add_supported_swing_mode(climate::CLIMATE_SWING_BOTH);
  if (this->supportsVaneSwing())
    ct.add_supported_swing_mode(climate::CLIMATE_SWING_VERTICAL);
  if (this->supportsHVaneSwing())
    ct.add_supported_swing_mode(climate::CLIMATE_SWING_HORIZONTAL);

  ct.set_visual_min_temperature(min(this->getMinCoolDrySetpoint(), this->getMinHeatingSetpoint()));
  ct.set_visual_max_temperature(max(this->getMaxCoolDrySetpoint(), this->getMaxHeatingSetpoint()));

  // TODO: Figure out what these states *actually* map to so we aren't sending bad data.
  // This is probably a dynamic map, so the setter will need to be aware of things.
  switch (this->getSupportedFanSpeeds()) {
    case 1:
      ct.set_supported_fan_modes({climate::CLIMATE_FAN_HIGH});
      break;
    case 2:
      ct.set_supported_fan_modes({climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_HIGH});
      break;
    case 3:
      ct.set_supported_fan_modes({
        climate::CLIMATE_FAN_LOW,
        climate::CLIMATE_FAN_MEDIUM,
        climate::CLIMATE_FAN_HIGH
      });
      break;
    case 4:
      ct.set_supported_fan_modes({
          climate::CLIMATE_FAN_QUIET,
          climate::CLIMATE_FAN_LOW,
          climate::CLIMATE_FAN_MEDIUM,
          climate::CLIMATE_FAN_HIGH,
      });
      break;
    case 5:
      ct.set_supported_fan_modes({
          climate::CLIMATE_FAN_QUIET,
          climate::CLIMATE_FAN_LOW,
          climate::CLIMATE_FAN_MEDIUM,
          climate::CLIMATE_FAN_HIGH,
      });
      ct.add_supported_custom_fan_mode("Very High");
      break;
    default:
      // no-op, don't set a fan mode.
      break;
  }
  if (!this->autoFanSpeedDisabled()) ct.add_supported_fan_mode(climate::CLIMATE_FAN_AUTO);

  return ct;
}

// ErrorStateGetResponsePacket functions
std::string ErrorStateGetResponsePacket::getShortCode() const {
  const auto upperAlphabet = "AbEFJLPU";
  const auto lowerAlphabet = "0123456789ABCDEFOHJLPU";
  const auto errorCode = this->getRawShortCode();

  auto lowBits = errorCode & 0x1F;
  if (lowBits > 0x15) {
    ESP_LOGW(PACKETS_TAG, "Error lowbits %x were out of range.", lowBits);
    char buf[7];
    sprintf(buf, "ERR_%x", errorCode);
    return buf;
  }

  return {upperAlphabet[(errorCode & 0xE0) >> 5], lowerAlphabet[lowBits]};
}


}  // namespace mitsubishi_uart
}  // namespace esphome
