#include "muart_packet.h"

namespace esphome {
namespace mitsubishi_uart {

// Packet to_strings()

std::string ConnectRequestPacket::to_string() const {
  return("Connect Request: " + Packet::to_string());
}
std::string ConnectResponsePacket::to_string() const {
  return("Connect Response: " + Packet::to_string());
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
  + " Power:" + (getPower() ? "On" : "Off")
  + " TargetTemp:" + std::to_string(getTargetTemp())
  + " Vane:" + format_hex(getVane())
  + " HVane:" + format_hex(getHorizontalVane())
  );
}
std::string StandbyGetResponsePacket::to_string() const {
  return ("Standby Response: " + Packet::to_string()
  + CONSOLE_COLOR_PURPLE
  + "\n Loop Status:" + format_hex(getLoopStatus())
  + " Stage:" + format_hex(getStage())
  );
}
std::string StatusGetResponsePacket::to_string() const {
  return ("Status Response: " + Packet::to_string()
  + CONSOLE_COLOR_PURPLE
  + "\n CompressorFrequency: " + std::to_string(getCompressorFrequency())
  + " Operating: " + (getOperating() ? "Yes":"No")
  );
}
std::string RemoteTemperatureSetRequestPacket::to_string() const {
  return ("Remote Temp Set Request: " + Packet::to_string()
  + CONSOLE_COLOR_PURPLE
  + "\n Temp:" + std::to_string(getRemoteTemperature()));
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

}  // namespace mitsubishi_uart
}  // namespace esphome
