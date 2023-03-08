#include "mitsubishi_uart.h"

namespace esphome {
namespace mitsubishi_uart {

const char *TAG = "mitsubishi_uart";

const Packet PACKET_CONNECT_REQ = Packet(PKTTYPE_CONNECT_REQUEST, 2)
  .setPayloadByte(0,0xca).setPayloadByte(1,0x01);
const Packet PACKET_TEMP_REQ = Packet(PKTTYPE_GET_REQUEST, 1)
.setPayloadByte(0,0x03);

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

////
// MitsubishiUART
////

MitsubishiUART::MitsubishiUART(uart::UARTComponent *uart_comp) :
  hp_uart{uart_comp}
{
    this->_traits.set_supports_action(false);
    this->_traits.set_supports_current_temperature(true);
    this->_traits.set_supports_two_point_target_temperature(false);
    this->_traits.set_visual_min_temperature(MUART_MIN_TEMP);
    this->_traits.set_visual_max_temperature(MUART_MAX_TEMP);
    this->_traits.set_visual_temperature_step(MUART_TEMPERATURE_STEP);
}

void MitsubishiUART::setup() {
  connect();
}

climate::ClimateTraits MitsubishiUART::traits() { return _traits; }

void MitsubishiUART::control(const climate::ClimateCall &call) {

}

void MitsubishiUART::update() {
  ESP_LOGD(TAG, "Update called.");

  if (connectState == 2) {
    // Request room temp
    sendPacket(PACKET_TEMP_REQ);
  }

  // Read packets, but if we don't get any, keep track of how long it's been
  if (readPackets()) {
    updatesSinceLastPacket = 0;
  } else {
    if (++updatesSinceLastPacket > 10) {
      ESP_LOGI(TAG, "No packets received in 10 updates, connection down.");
      connectState = 0;
    }
  }

  this->publish_state();

  // If we're not connected (or have become unconnected) try to send a connect packet again
  if (connectState < 2) {connect();}
}

void MitsubishiUART::dump_config() {
  ESP_LOGCONFIG(TAG, "Mitsubishi UART v%s", MUART_VERSION);
  ESP_LOGCONFIG(TAG, "Connection state: %d", connectState);
}

void MitsubishiUART::connect() {
  sendPacket(PACKET_CONNECT_REQ);
  connectState = 1; // Connecting...
}

void MitsubishiUART::sendPacket(Packet packet) {
  hp_uart->write_array(packet.getBytes(),packet.getLength());
}

/**
 * Reads packets from UART, and sends them to appropriate handler methods.
*/
bool MitsubishiUART::readPackets() {
  uint8_t p_byte;
  bool foundPacket = false;

  // Search for control byte (or check that one is waiting for us)
  while (hp_uart->available() && hp_uart->peek_byte(&p_byte)){
    if (p_byte == BYTE_CONTROL){
      foundPacket=true; 
      ESP_LOGD(TAG, "FoundPacket!");
      break;
      }
    else {
      hp_uart->read_byte(&p_byte);
      }
  }

  // If control byte has been found and there's at least a header available, parse the packet
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

    switch (packet.getType()) {
      case PKTTYPE_CONNECT_RESPONSE:
        hResConnect(packet);
        break;
      case PKTTYPE_GET_RESPONSE:
        hResGet(packet);
        break;
      default:
        ESP_LOGI(TAG, "Unknown packet type %x received.", packet.getType());
    }

    return true;
  }

  return false;
}

void MitsubishiUART::hResConnect(Packet &packet){
  // Not sure there's any info in the response.
  connectState = 2;
  ESP_LOGI(TAG, "Connected to heatpump.");
}

void MitsubishiUART::hResGet(Packet &packet){
  switch (packet.getCommand()) {
    case 0x03: {
        float roomTemp = ((int)packet.getBytes()[PAYLOAD_INDEX_ROOMTEMP] - 128)/2.0f;
        this->current_temperature = roomTemp;
        ESP_LOGD(TAG, "Room temp: %.1f", roomTemp);
      }
      break;
    default:
      ESP_LOGI(TAG, "Unknown get response command %x received.", packet.getType());
  }
}


}  // namespace mitsubishi_uart
}  // namespace esphome