#include "muart_packet.h"

namespace esphome {
namespace mitsubishi_uart {

Packet::Packet(uint8_t packet_type, uint8_t payload_size)
    : length{payload_size + PACKET_HEADER_SIZE + 1}, checksumIndex{length - 1} {
  memcpy(packetBytes, EMPTY_PACKET, length);
  packetBytes[PACKET_HEADER_INDEX_PACKET_TYPE] = packet_type;
  packetBytes[PACKET_HEADER_INDEX_PAYLOAD_SIZE] = payload_size;

  updateChecksum();
}

Packet::Packet(uint8_t packet_header[PACKET_HEADER_SIZE], uint8_t payload[], uint8_t payload_size, uint8_t checksum)
    : length{payload_size + PACKET_HEADER_SIZE + 1}, checksumIndex{length - 1} {
  memcpy(packetBytes, packet_header, PACKET_HEADER_SIZE);
  memcpy(packetBytes + PACKET_HEADER_SIZE, payload, payload_size);
  packetBytes[checksumIndex] = checksum;

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

const bool Packet::isChecksumValid() const { return packetBytes[checksumIndex] == calculateChecksum(); }

Packet &Packet::setPayloadByte(int payload_byte_index, uint8_t value) {
  packetBytes[PACKET_HEADER_SIZE + payload_byte_index] = value;
  updateChecksum();
  return *this;
}

}  // namespace mitsubishi_uart
}  // namespace esphome
