#include "muart_packet.h"

namespace esphome {
namespace mitsubishi_uart {

Packet &Packet::setPayloadByte(const uint8_t payload_byte_index, const uint8_t value){
  pkt_.setPayloadByte(payload_byte_index, value);
  return *this;
};

// Packet::Packet(Packet&& pkt) :
//   length(std::move(pkt.length)),
//   checksumIndex(std::move(pkt.checksumIndex)),
//   responseExpected(std::move(pkt.responseExpected)) {}

// Creates an empty packet
Packet::Packet() {
  // TODO: Is this okay?
}

// Packet to_strings()

std::string ConnectRequestPacket::to_string() const {
  return("Connect Request: " + Packet::to_string());
}
std::string ConnectResponsePacket::to_string() const {
  return("Connect Response: " + Packet::to_string());
}
std::string CurrentTempGetResponsePacket::to_string() const {
  return ("Current Temp Response:" + Packet::to_string() + "\n Temp: " + std::to_string(getCurrentTemp()));
}

// TODO: Are there function implementations for packets in the .h file? (Yes)  Should they be here?

// SettingsSetRequestPacket functions

void SettingsSetRequestPacket::addSettingsFlag(const SETTING_FLAG flagToAdd) {
  pkt_.addFlag(flagToAdd);
}

void SettingsSetRequestPacket::addSettingsFlag2(const SETTING_FLAG2 flag2ToAdd) {
  pkt_.addFlag2(flag2ToAdd);
}

SettingsSetRequestPacket &SettingsSetRequestPacket::setPower(const bool isOn) {
  setPayloadByte(PLINDEX_POWER, isOn ? 0x01 : 0x00);
  addSettingsFlag(SF_POWER);
  return *this;
}

SettingsSetRequestPacket &SettingsSetRequestPacket::setMode(const MODE_BYTE mode) {
  setPayloadByte(PLINDEX_MODE, mode);
  addSettingsFlag(SF_MODE);
  return *this;
}

SettingsSetRequestPacket &SettingsSetRequestPacket::setTargetTemperature(const float temperatureDegressC) {
  if (temperatureDegressC < 63.5 && temperatureDegressC > -64.0) {
    setPayloadByte(PLINDEX_TARGET_TEMPERATURE, round(temperatureDegressC * 2) + 128);
    addSettingsFlag(SF_TARGET_TEMPERATURE);
  } else {
    ESP_LOGW(PTAG, "Target temp %f is outside valid range.", temperatureDegressC);
  }
  return *this;
}
SettingsSetRequestPacket &SettingsSetRequestPacket::setFan(const FAN_BYTE fan) {
  setPayloadByte(PLINDEX_FAN, fan);
  addSettingsFlag(SF_FAN);
  return *this;
}

SettingsSetRequestPacket &SettingsSetRequestPacket::setVane(const VANE_BYTE vane) {
  setPayloadByte(PLINDEX_VANE, vane);
  addSettingsFlag(SF_VANE);
  return *this;
}

SettingsSetRequestPacket &SettingsSetRequestPacket::setHorizontalVane(const HORIZONTAL_VANE_BYTE horizontal_vane) {
  setPayloadByte(PLINDEX_HORIZONTAL_VANE, horizontal_vane);
  addSettingsFlag2(SF2_HORIZONTAL_VANE);
  return *this;
}


// RemoteTemperatureSetRequestPacket functions

RemoteTemperatureSetRequestPacket &RemoteTemperatureSetRequestPacket::setRemoteTemperature(float temperatureDegressC) {
  if (temperatureDegressC < 63.5 && temperatureDegressC > -64.0) {
    setPayloadByte(PLINDEX_REMOTE_TEMPERATURE, round(temperatureDegressC * 2) + 128);
    pkt_.setFlags(0x01); // Set flags to say we're providing the temperature
  } else {
    ESP_LOGW(PTAG, "Remote temp %f is outside valid range.", temperatureDegressC);
  }
  return *this;
}
RemoteTemperatureSetRequestPacket &RemoteTemperatureSetRequestPacket::useInternalTemperature() {
  pkt_.setFlags(0x00);  // Set flags to say to use internal temperature
  return *this;
}

}  // namespace mitsubishi_uart
}  // namespace esphome
