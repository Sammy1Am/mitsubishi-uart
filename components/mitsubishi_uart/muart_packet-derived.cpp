#include "muart_packet.h"
#include "muart_utils.h"
#include "mitsubishi_uart.h"

namespace esphome {
namespace mitsubishi_uart {

// Packet to_strings()

std::string ConnectRequestPacket::to_string() const {
  return("Connect Request: " + Packet::to_string());
}
std::string ConnectResponsePacket::to_string() const {
  return("Connect Response: " + Packet::to_string());
}
std::string ExtendedConnectResponsePacket::to_string() const {
  return ("Extended Connect Response: " + Packet::to_string()
  + CONSOLE_COLOR_PURPLE
  + "\n HeatDisabled:" + (isHeatDisabled()?"Yes":"No")
  + " SupportsVane:" + (supportsVane()?"Yes":"No")
  + " SupportsVaneSwing:" + (supportsVaneSwing()?"Yes":"No")

  + " DryDisabled:" + (isDryDisabled()?"Yes":"No")
  + " FanDisabled:" + (isFanDisabled()?"Yes":"No")
  + " ExtTempRange:" + (hasExtendedTemperatureRange()?"Yes":"No")
  + " AutoFan:" + (hasAutoFanSpeed()?"Yes":"No")
  + " InstallerSettings:" + (supportsInstallerSettings()?"Yes":"No")
  + " TestMode:" + (supportsTestMode()?"Yes":"No")
  + " DryTemp:" + (supportsDryTemperature()?"Yes":"No")

  + " StatusDisplay:" + (hasStatusDisplay()?"Yes":"No")

  + "\n CoolDrySetpoint:" + std::to_string(getMinCoolDrySetpoint()) + "/" + std::to_string(getMaxCoolDrySetpoint())
  + " HeatSetpoint:" + std::to_string(getMinHeatingSetpoint()) + "/" + std::to_string(getMaxHeatingSetpoint())
  + " AutoSetpoint:" + std::to_string(getMinAutoSetpoint()) + "/" + std::to_string(getMaxAutoSetpoint())
  + " FanSpeeds:" + std::to_string(getSupportedFanSpeeds()));
}
std::string CurrentTempGetResponsePacket::to_string() const {
  return ("Current Temp Response: " + Packet::to_string()
  + CONSOLE_COLOR_PURPLE
  + "\n Temp:" + std::to_string(getCurrentTemp()));
}
std::string SettingsGetResponsePacket::to_string() const {

  return ("Settings Response: " + Packet::to_string()
  + CONSOLE_COLOR_PURPLE
  + "\n Fan:" + format_hex(getFan())
  + " Mode:" + format_hex(getMode())
  + " Power:" + (getPower()==3 ? "Test" : getPower()>0 ? "On" : "Off")
  + " TargetTemp:" + std::to_string(getTargetTemp())
  + " Vane:" + format_hex(getVane())
  + " HVane:" + format_hex(getHorizontalVane())
  + "\n PowerLock:" + (lockedPower()?"Yes":"No")
  + " ModeLock:" + (lockedMode()?"Yes":"No")
  + " TempLock:" + (lockedTemp()?"Yes":"No")
  );
}
std::string StandbyGetResponsePacket::to_string() const {
  return ("Standby Response: " + Packet::to_string()
  + CONSOLE_COLOR_PURPLE
  + "\n ServiceFilter:" + (serviceFilter()?"Yes":"No")
  + " Defrost:" + (inDefrost()?"Yes":"No")
  + " HotAdjust:" + (inHotAdjust()?"Yes":"No")
  + " Standby:" + (inStandby()?"Yes":"No")
  + " ActualFan:" + std::to_string(getActualFanSpeed())
  + " AutoMode:" + format_hex(getAutoMode())
  );
}
std::string StatusGetResponsePacket::to_string() const {
  return ("Status Response: " + Packet::to_string()
  + CONSOLE_COLOR_PURPLE
  + "\n CompressorFrequency: " + std::to_string(getCompressorFrequency())
  + " Operating: " + (getOperating() ? "Yes":"No")
  );
}
std::string ErrorStateGetResponsePacket::to_string() const {
  return ("Error State Response: " + Packet::to_string()
  + CONSOLE_COLOR_PURPLE
  + "\n Error State: " + (errorPresent() ? "Yes" : "No")
  + " ErrorCode: " + format_hex(getErrorCode())
  + " ShortCode: " + getShortCode() + "(" + format_hex(getRawShortCode()) + ")"
  );
}
std::string RemoteTemperatureSetRequestPacket::to_string() const {
  return ("Remote Temp Set Request: " + Packet::to_string()
  + CONSOLE_COLOR_PURPLE
  + "\n Temp:" + std::to_string(getRemoteTemperature()));
}

std::string ThermostatHelloRequestPacket::to_string() const {
  return("Thermostat Hello: " + Packet::to_string() + CONSOLE_COLOR_PURPLE +
          "\n Model: " + getThermostatModel() +
          " Serial: " + getThermostatSerial() +
          " Version: " + getThermostatVersionString());
}

// TODO: Are there function implementations for packets in the .h file? (Yes)  Should they be here?

// SettingsSetRequestPacket functions

void SettingsSetRequestPacket::addSettingsFlag(const SETTING_FLAG flagToAdd) {
  addFlag(flagToAdd);
}

void SettingsSetRequestPacket::addSettingsFlag2(const SETTING_FLAG2 flag2ToAdd) {
  addFlag2(flag2ToAdd);
}

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

// SettingsGetResponsePacket functions
float SettingsGetResponsePacket::getTargetTemp() const {
  uint8_t enhancedRawTemp = pkt_.getPayloadByte(PLINDEX_TARGETTEMP);

  if (enhancedRawTemp == 0x00) {
    uint8_t legacyRawTemp = pkt_.getPayloadByte(PLINDEX_TARGETTEMP_LEGACY);
    return ((float)(31 - (legacyRawTemp % 0x10)) + (0.5f * (float)(legacyRawTemp & 0x10)));
  }

  return ((float)enhancedRawTemp - 128) / 2.0f;
}


// RemoteTemperatureSetRequestPacket functions

RemoteTemperatureSetRequestPacket &RemoteTemperatureSetRequestPacket::setRemoteTemperature(float temperatureDegressC) {
  if (temperatureDegressC < 63.5 && temperatureDegressC > -64.0) {
    pkt_.setPayloadByte(PLINDEX_REMOTE_TEMPERATURE, round(temperatureDegressC * 2) + 128);
    setFlags(0x01); // Set flags to say we're providing the temperature
  } else {
    ESP_LOGW(PTAG, "Remote temp %f is outside valid range.", temperatureDegressC);
  }
  return *this;
}
RemoteTemperatureSetRequestPacket &RemoteTemperatureSetRequestPacket::useInternalTemperature() {
  setFlags(0x00);  // Set flags to say to use internal temperature
  return *this;
}

// CurrentTempGetResponsePacket functions
float CurrentTempGetResponsePacket::getCurrentTemp() const {
  uint8_t enhancedRawTemp = pkt_.getPayloadByte(PLINDEX_CURRENTTEMP);

  //TODO: Figure out how to handle "out of range" issues here.
  if (enhancedRawTemp == 0)
    return 8 + ((float) pkt_.getPayloadByte(PLINDEX_CURRENTTEMP_LEGACY) * 0.5f);

  return ((float) enhancedRawTemp - 128) / 2.0f;
}

// ThermostatHelloRequestPacket functions
std::string ThermostatHelloRequestPacket::getThermostatModel() const {
  return MUARTUtils::DecodeNBitString((pkt_.getBytes() + 1), 3, 6);
}

std::string ThermostatHelloRequestPacket::getThermostatSerial() const {
  return MUARTUtils::DecodeNBitString((pkt_.getBytes() + 4), 8, 6);
}

std::string ThermostatHelloRequestPacket::getThermostatVersionString() const {
  char buf[16];
  sprintf(buf, "%02d.%02d.%02d",
          pkt_.getPayloadByte(13),
          pkt_.getPayloadByte(14),
          pkt_.getPayloadByte(15));

  return buf;
}

// ErrorStateGetResponsePacket functions
std::string ErrorStateGetResponsePacket::getShortCode() const {
  const auto upperAlphabet = "AbEFJLPU";
  const auto lowerAlphabet = "0123456789ABCDEFOHJLPU";
  const auto errorCode = this->getRawShortCode();

  auto lowBits = errorCode & 0x1F;
  if (lowBits > 0x15) {
    char buf[7];
    sprintf(buf, "ERR_%x", errorCode);
    return buf;
  }

  return {upperAlphabet[(errorCode & 0xE0) >> 5], lowerAlphabet[lowBits]};
}

}  // namespace mitsubishi_uart
}  // namespace esphome
