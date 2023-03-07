#include "mitsubishi_uart.h"

namespace esphome {
namespace mitsubishi_uart {

static const char *TAG = "mitsubishi_uart";

////
// Packet
////
Packet::Packet(uint8_t packetType, uint8_t payloadSize) :
  length{payloadSize + HEADER_SIZE + 1},
  checksumIndex{length-1},
  packetBytes{new uint8_t[length]()}
{
  memcpy(packetBytes,EMPTY_PACKET,length);
  packetBytes[HEADER_INDEX_PACKET_TYPE] = packetType;
  packetBytes[HEADER_INDEX_PAYLOAD_SIZE] = payloadSize;

  updateChecksum();
}

void Packet::updateChecksum() {
  uint8_t sum = 0;
  for (int i = 0; i < checksumIndex; i++) {
    sum += packetBytes[i];
  }

  packetBytes[checksumIndex] = (0xfc - sum) & 0xff;
}

void Packet::setPayloadByte(int payloadByteIndex, uint8_t value) {
  packetBytes[HEADER_SIZE + payloadByteIndex] = value;
  updateChecksum();
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

MitsubishiUART::MitsubishiUART(uart::UARTComponent *uart_comp) {
    this->_hp_uart = uart_comp;
}

void MitsubishiUART::setup() {
  
}

climate::ClimateTraits MitsubishiUART::traits() { return _traits; }

void MitsubishiUART::control(const climate::ClimateCall &call) {

}

void MitsubishiUART::update() {
  connect();
  ESP_LOGI(TAG, "Update!");
}

void MitsubishiUART::dump_config() {
  ESP_LOGCONFIG(TAG, "Config dump!");
}

void MitsubishiUART::connect() {
  Packet conn_packet = Packet(PKTTYPE_CONNECT,2);
  conn_packet.setPayloadByte(0,0xca);
  conn_packet.setPayloadByte(1,0x01);
  _hp_uart->write_array(conn_packet.getBytes(),conn_packet.getLength());
}

}  // namespace mitsubishi_uart
}  // namespace esphome