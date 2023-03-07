#include "mitsubishi_uart.h"

namespace esphome {
namespace mitsubishi_uart {

const char *TAG = "mitsubishi_uart";

const Packet PACKET_CONNECT = Packet(BYTE_PKTTYPE_CONNECT, 2)
  .setPayloadByte(0,0xca).setPayloadByte(1,0x01);

////
// Packet
////
Packet::Packet(uint8_t packet_type, uint8_t payload_size)
    : length{payload_size + HEADER_SIZE + 1}, checksumIndex{length - 1} {
  memcpy(packetBytes,EMPTY_PACKET,length);
  packetBytes[HEADER_INDEX_PACKET_TYPE] = packet_type;
  packetBytes[HEADER_INDEX_PAYLOAD_SIZE] = payload_size;

  updateChecksum();
}

Packet::Packet(uint8_t packet_header[HEADER_SIZE], uint8_t payload[], uint8_t payload_size, uint8_t checksum)
    : length{payload_size + HEADER_SIZE + 1}, checksumIndex{length - 1} {
  memcpy(packetBytes,packet_header,HEADER_SIZE);
  memcpy(packetBytes+HEADER_SIZE, payload, payload_size);
  packetBytes[checksumIndex] = checksum;
}

const uint8_t Packet::calculateChecksum() {
  uint8_t sum = 0;
  for (int i = 0; i < checksumIndex; i++) {
    sum += packetBytes[i];
  }

  return (0xfc - sum) & 0xff;
}

Packet& Packet::updateChecksum() {
  packetBytes[checksumIndex] = calculateChecksum();
  return *this;
}

const bool Packet::isChecksumValid() {
  return packetBytes[checksumIndex] == calculateChecksum();
}

Packet &Packet::setPayloadByte(int payload_byte_index, uint8_t value) {
  packetBytes[HEADER_SIZE + payload_byte_index] = value;
  updateChecksum();
  return *this;
}

// Packet PACKET_CONNECT = []() -> Packet {
//   Packet packet = Packet(PKTTYPE_CONNECT,2);
//   packet.setPayloadByte(0,0xca);
//   packet.setPayloadByte(1,0x01);
//   return packet;
// }();

////
// MitsubishiUART
////

MitsubishiUART::MitsubishiUART(uart::UARTComponent *uart_comp) :
  hp_uart{uart_comp}
{
    //this->_hp_uart = uart_comp;
}

void MitsubishiUART::setup() {
  
}

climate::ClimateTraits MitsubishiUART::traits() { return _traits; }

void MitsubishiUART::control(const climate::ClimateCall &call) {

}

void MitsubishiUART::update() {
  readPackets();
  connect();
  ESP_LOGI(TAG, "Update!");
}

void MitsubishiUART::dump_config() {
  ESP_LOGCONFIG(TAG, "Config dump!");
}

void MitsubishiUART::connect() {
  sendPacket(PACKET_CONNECT);
}

void MitsubishiUART::sendPacket(Packet packet) {
  hp_uart->write_array(packet.getBytes(),packet.getLength());
}

void MitsubishiUART::readPackets() {
  uint8_t p_byte;
  bool foundPacket = false;
  while (hp_uart->available() && hp_uart->peek_byte(&p_byte)){
    if (p_byte == BYTE_CONTROL){
      foundPacket=true; 
      ESP_LOGD(TAG, "FoundPacket!");
      break;}
  }

  if (foundPacket && hp_uart->available() > HEADER_SIZE) {
    uint8_t p_header[HEADER_SIZE];
    hp_uart->read_array(p_header, HEADER_SIZE);
    int payloadSize = p_header[HEADER_INDEX_PAYLOAD_SIZE];
    uint8_t p_payload[payloadSize];
    uint8_t checksum;
    hp_uart->read_array(p_payload,payloadSize);
    hp_uart->read_byte(&checksum);

    Packet packet = Packet(p_header,p_payload,payloadSize,checksum);
    ESP_LOGD(TAG, "Packet with type %x and %s validity.", packet.getType(), YESNO(packet.isChecksumValid()));
  }
}

}  // namespace mitsubishi_uart
}  // namespace esphome