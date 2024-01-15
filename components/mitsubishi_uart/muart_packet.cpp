#include "muart_packet.h"

namespace esphome {
namespace mitsubishi_uart {

// Creates an empty packet
Packet::Packet(PacketType packet_type, uint8_t payload_size)
    : length{(uint8_t)(payload_size + PACKET_HEADER_SIZE + 1)}, checksumIndex{(uint8_t)(length - 1)} {
  memcpy(packetBytes, EMPTY_PACKET, length);
  packetBytes[PACKET_HEADER_INDEX_PACKET_TYPE] = packet_type;
  packetBytes[PACKET_HEADER_INDEX_PAYLOAD_SIZE] = payload_size;

  updateChecksum();
}

// Creates a packet with the provided bytes
Packet::Packet(const uint8_t packet_bytes[], const uint8_t packet_length)
    : length{(uint8_t) packet_length}, checksumIndex{(uint8_t)(packet_length - 1)} {
  memcpy(packetBytes, packet_bytes, packet_length);

  if (!this->isChecksumValid()) {
    // For now, just log this as information (we can decide if we want to process it elsewhere)
    ESP_LOGI(PTAG, "Packet of type %x has invalid checksum!", this->getPacketType());
  }
}

uint8_t Packet::calculateChecksum() const {
  uint8_t sum = 0;
  for (int i = 0; i < checksumIndex; i++) {
    sum += packetBytes[i];
  }

  return (0xfc - sum) & 0xff;
}

Packet &Packet::updateChecksum() {
  packetBytes[checksumIndex] = calculateChecksum();
  return *this;
}

bool Packet::isChecksumValid() const { return packetBytes[checksumIndex] == calculateChecksum(); }

// Sets a payload byte and automatically updates the packet checksum
Packet &Packet::setPayloadByte(uint8_t payload_byte_index, uint8_t value) {
  packetBytes[PACKET_HEADER_SIZE + payload_byte_index] = value;
  updateChecksum();
  return *this;
}

// SettingsSetRequestPacket functions

void SettingsSetRequestPacket::addFlag(const SETTING_FLAG flagToAdd) {
  setPayloadByte(PLINDEX_FLAGS, getPayloadByte(PLINDEX_FLAGS) | flagToAdd);
}

void SettingsSetRequestPacket::addFlag2(const SETTING_FLAG2 flag2ToAdd) {
  setPayloadByte(PLINDEX_FLAGS2, getPayloadByte(PLINDEX_FLAGS2) | flag2ToAdd);
}

SettingsSetRequestPacket &SettingsSetRequestPacket::setPower(const bool isOn) {
  setPayloadByte(PLINDEX_POWER, isOn ? 0x01 : 0x00);
  addFlag(SF_POWER);
  return *this;
}

SettingsSetRequestPacket &SettingsSetRequestPacket::setMode(const MODE_BYTE mode) {
  setPayloadByte(PLINDEX_MODE, mode);
  addFlag(SF_MODE);
  return *this;
}

SettingsSetRequestPacket &SettingsSetRequestPacket::setTargetTemperature(const float temperatureDegressC) {
  if (temperatureDegressC < 63.5 && temperatureDegressC > -64.0) {
    setPayloadByte(PLINDEX_TARGET_TEMPERATURE, round(temperatureDegressC * 2) + 128);
    addFlag(SF_TARGET_TEMPERATURE);
  } else {
    ESP_LOGW(PTAG, "Target temp %f is outside valid range.", temperatureDegressC);
  }
  return *this;
}
SettingsSetRequestPacket &SettingsSetRequestPacket::setFan(const FAN_BYTE fan) {
  setPayloadByte(PLINDEX_FAN, fan);
  addFlag(SF_FAN);
  return *this;
}

SettingsSetRequestPacket &SettingsSetRequestPacket::setVane(const VANE_BYTE vane) {
  setPayloadByte(PLINDEX_VANE, vane);
  addFlag(SF_VANE);
  return *this;
}

SettingsSetRequestPacket &SettingsSetRequestPacket::setHorizontalVane(const HORIZONTAL_VANE_BYTE horizontal_vane) {
  setPayloadByte(PLINDEX_HORIZONTAL_VANE, horizontal_vane);
  addFlag2(SF2_HORIZONTAL_VANE);
  return *this;
}


// RemoteTemperatureSetRequestPacket functions

RemoteTemperatureSetRequestPacket &RemoteTemperatureSetRequestPacket::setRemoteTemperature(float temperatureDegressC) {
  if (temperatureDegressC < 63.5 && temperatureDegressC > -64.0) {
    setPayloadByte(PLINDEX_REMOTE_TEMPERATURE, round(temperatureDegressC * 2) + 128);
    setPayloadByte(Packet::PLINDEX_FLAGS, 0x01);  // Set flags to say we're providing the temperature
  } else {
    ESP_LOGW(PTAG, "Remote temp %f is outside valid range.", temperatureDegressC);
  }
  return *this;
}
RemoteTemperatureSetRequestPacket &RemoteTemperatureSetRequestPacket::useInternalTemperature() {
  setPayloadByte(Packet::PLINDEX_FLAGS, 0x00);  // Set flags to say to use internal temperature
  return *this;
}

}  // namespace mitsubishi_uart
}  // namespace esphome
