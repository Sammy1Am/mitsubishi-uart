#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace mitsubishi_uart {

static const int HEADER_SIZE = 5;
static const int HEADER_INDEX_PACKET_TYPE = 1;
static const int HEADER_INDEX_PAYLOAD_SIZE = 4;

static const uint8_t EMPTY_PACKET[22] = {0xfc, // Sync
0x00, // Packet type
0x01, 0x30, // Unknown
0x00, // Payload Size
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Payload
0x00}; // Checksum

static const uint8_t PKTTYPE_CONNECT = 0x5a;

class Packet {
  public:
    Packet(uint8_t packetType, uint8_t payloadSize);
    const uint8_t* getBytes() {return packetBytes;};
    const int getLength() {return length;};
    void setPayloadByte(int payloadByteIndex, uint8_t value);
  private:
    const int length;
    const int checksumIndex;
    uint8_t* packetBytes;
    void updateChecksum();
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
  uart::UARTComponent *_hp_uart {nullptr};
  climate::ClimateTraits _traits;
  void connect();
};

}  // namespace mitsubishi_uart
}  // namespace esphome