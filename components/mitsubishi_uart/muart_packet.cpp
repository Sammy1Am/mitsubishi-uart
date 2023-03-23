#include "muart_packet.h"

namespace esphome {
namespace mitsubishi_uart {

Packet::Packet(uint8_t packet_type, uint8_t payload_size)
    : length{(uint8_t)(payload_size + PACKET_HEADER_SIZE + 1)}, checksumIndex{(uint8_t)(length - 1)} {
  memcpy(packetBytes, EMPTY_PACKET, length);
  packetBytes[PACKET_HEADER_INDEX_PACKET_TYPE] = packet_type;
  packetBytes[PACKET_HEADER_INDEX_PAYLOAD_SIZE] = payload_size;

  updateChecksum();
}

Packet::Packet(const uint8_t packet_bytes[], const uint8_t packet_length)
    : length{(uint8_t) packet_length}, checksumIndex{(uint8_t)(packet_length - 1)} {
  memcpy(packetBytes, packet_bytes, packet_length);

  if (!this->isChecksumValid()) {
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

Packet &Packet::setPayloadByte(uint8_t payload_byte_index, uint8_t value) {
  packetBytes[PACKET_HEADER_SIZE + payload_byte_index] = value;
  updateChecksum();
  return *this;
}

PacketSetRemoteTemperatureRequest &PacketSetRemoteTemperatureRequest::setRemoteTemperature(float temperatureDegressC) {
  if (temperatureDegressC < 63.5 && temperatureDegressC > -64.0) {
    setPayloadByte(PAYLOAD_INDEX_REMOTE_TEMPERATURE, round(temperatureDegressC * 2) + 128);
    setPayloadByte(Packet::PAYLOAD_INDEX_FLAGS, 0x01);  // Set flags to say we're providing the temperature
  } else {
    ESP_LOGW(PTAG, "Remote temp %f is outside valid range.", temperatureDegressC);
  }
  return *this;
}
PacketSetRemoteTemperatureRequest &PacketSetRemoteTemperatureRequest::useInternalTemperature() {
  setPayloadByte(Packet::PAYLOAD_INDEX_FLAGS, 0x01);  // Set flags to say to use internal temperature
  return *this;
}

}  // namespace mitsubishi_uart
}  // namespace esphome
