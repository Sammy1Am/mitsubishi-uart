#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace mitsubishi_uart {

const int PACKET_MAX_SIZE = 22; // Used to intialize blank packets

const int HEADER_SIZE = 5;
const int HEADER_INDEX_PACKET_TYPE = 1;
const int HEADER_INDEX_PAYLOAD_SIZE = 4;

const uint8_t BYTE_CONTROL = 0xfc;

const uint8_t EMPTY_PACKET[PACKET_MAX_SIZE] = {BYTE_CONTROL, // Sync
0x00, // Packet type
0x01, 0x30, // Unknown
0x00, // Payload Size
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Payload
0x00}; // Checksum



const uint8_t BYTE_PKTTYPE_CONNECT = 0x5a;

class Packet {
  public:
    // Constructors
    Packet(uint8_t packet_type, uint8_t payload_size); // For building packets
    Packet(uint8_t packet_header[HEADER_SIZE], uint8_t payload[], uint8_t payload_size, uint8_t checksum); // For reading packets
    const uint8_t *getBytes() {return packetBytes;}; // Primarily for sending packets
    const int getLength() {return length;};
    Packet &setPayloadByte(int payload_byte_index, uint8_t value);
    const bool isChecksumValid();

    // Packet information getters
    const uint8_t getType(){return packetBytes[HEADER_INDEX_PACKET_TYPE];};
  private:
    const int length;
    const int checksumIndex;
    uint8_t packetBytes[PACKET_MAX_SIZE]{};
    const uint8_t calculateChecksum();
    Packet &updateChecksum();
    
};

class MitsubishiUART : public climate::Climate, public PollingComponent {
 public:
  /**
   * Create a new MitsubishiUART with the specified esphome::uart::UARTComponent.
   */
  MitsubishiUART(uart::UARTComponent *uart_comp);

  // Set up the component, initializing the HeatPump object.
  void setup() override;

  // ?
  climate::ClimateTraits traits() override;

  // ?
  void control(const climate::ClimateCall &call) override;

  // Called periodically as PollingComponent
  void update() override;

  void dump_config() override;

 private:
  uart::UARTComponent *hp_uart;
  climate::ClimateTraits _traits;
  void connect();
  void sendPacket(Packet packet);
  void readPackets();
};

}  // namespace mitsubishi_uart
}  // namespace esphome