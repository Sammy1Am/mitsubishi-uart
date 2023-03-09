#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace mitsubishi_uart {

static const char* MUART_VERSION = "0.1.0";

const int PACKET_MAX_SIZE = 22; // Used to intialize blank packets
const int PACKET_RECEIVE_TIMEOUT = 500; // Milliseconds to wait for a response

const int HEADER_SIZE = 5;
const int HEADER_INDEX_PACKET_TYPE = 1;
const int HEADER_INDEX_PAYLOAD_SIZE = 4;
const int PAYLOAD_INDEX_COMMAND = 5;

// Get temp
const int PAYLOAD_INDEX_ROOMTEMP_CODE = 8; // TODO: I don't know why I would use this instead of the one below...
const int PAYLOAD_INDEX_ROOMTEMP = 11;

// Get status
const int PAYLOAD_INDEX_OPERATING = 9;
const int PAYLOAD_INDEX_COMPRESSOR_FREQUENCY = 8;

// Get settings
const int PAYLOAD_INDEX_POWER = 8;
const int PAYLOAD_INDEX_ISEE = 9;
const int PAYLOAD_INDEX_MODE = 9;
const int PAYLOAD_INDEX_TARGETTEMP = 16;
const int PAYLOAD_INDEX_FAN = 11;
const int PAYLOAD_INDEX_VANE = 12;
const int PAYLOAD_INDEX_HVANE = 15;

const uint8_t BYTE_CONTROL = 0xfc;

const uint8_t EMPTY_PACKET[PACKET_MAX_SIZE] = {BYTE_CONTROL, // Sync
0x00, // Packet type
0x01, 0x30, // Unknown
0x00, // Payload Size
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Payload
0x00}; // Checksum

const uint8_t MUART_MIN_TEMP = 16; // Degrees C
const uint8_t MUART_MAX_TEMP = 31; // Degrees C
const float MUART_TEMPERATURE_STEP = 0.5;

const uint8_t PKTTYPE_SET_REQUEST = 0x41;
const uint8_t PKTTYPE_SET_RESPONSE = 0x61;
const uint8_t PKTTYPE_GET_REQUEST = 0x42;
const uint8_t PKTTYPE_GET_RESPONSE = 0x62;
const uint8_t PKTTYPE_CONNECT_REQUEST = 0x5a;
const uint8_t PKTTYPE_CONNECT_RESPONSE = 0x7a;
const uint8_t PKTTYPE_EXT_CONNECT_REQUEST = 0x5b;
const uint8_t PKTTYPE_EXT_CONNECT_RESPONSE = 0x5a;

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
    const uint8_t getCommand(){return packetBytes[PAYLOAD_INDEX_COMMAND];};
  private:
    const int length;
    const int checksumIndex;
    uint8_t packetBytes[PACKET_MAX_SIZE]{};
    const uint8_t calculateChecksum();
    Packet &updateChecksum();
    
};

struct muartState {
  climate::ClimateMode c_mode;
  float c_target_temperature;
  climate::ClimateFanMode c_fan_mode;
  float c_current_temperature;
  climate::ClimateAction c_action;
  
  // power
  // vane
  // hvane
  // operating
  // compressor_frequency
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
  climate::ClimateTraits& config_traits();

  // ?
  void control(const climate::ClimateCall &call) override;

  // Called periodically as PollingComponent
  void update() override;

  void dump_config() override;

 private:
  uart::UARTComponent *hp_uart;
  uint8_t updatesSinceLastPacket = 0;
  muartState lastPublishedState {};
  muartState getCurrentState();

  climate::ClimateTraits _traits;

  uint8_t connectState = 0;

  void connect();

  // Sends a packet and by default attempts to receive one.  Returns true if a packet was received
  // Caveat: No attempt is made to match received packet with sent packet
  bool sendPacket(Packet packet, bool expectResponse=true); 
  bool readPacket(bool waitForPacket=true);  //TODO separate methods or arguments for HP vs tstat?

  // Packet response handling
  void hResConnect(Packet &packet);
  void hResGet(Packet &packet);
};

}  // namespace mitsubishi_uart
}  // namespace esphome