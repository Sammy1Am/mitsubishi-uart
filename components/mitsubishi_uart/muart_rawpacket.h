#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace mitsubishi_uart {

static const char *PTAG = "mitsubishi_uart.packets";

const uint8_t BYTE_CONTROL = 0xfc;
const uint8_t PACKET_MAX_SIZE = 22;  // Used to intialize empty packet
const uint8_t PACKET_HEADER_SIZE = 5;
const uint8_t PACKET_HEADER_INDEX_PACKET_TYPE = 1;
const uint8_t PACKET_HEADER_INDEX_PAYLOAD_LENGTH = 4;

enum PacketType : uint8_t {
  connect_request = 0x5a,
  connect_response = 0x7a,
  get_request = 0x42,
  get_response = 0x62,
  set_request = 0x41,
  set_response = 0x61,
  extended_connect_request = 0x5b,
  extended_connect_response = 0x7b
};

// Used to specify certain packet subtypes
enum GetCommand : uint8_t {
  gc_settings = 0x02,
  gc_current_temp = 0x03,
  gc_four = 0x04,
  gc_status = 0x06,
  gc_standby = 0x09
};

// Used to specify certain packet subtypes
enum SetCommand : uint8_t {
  sc_settings = 0x01,
  sc_remote_temperature = 0x07
};

static const uint8_t EMPTY_PACKET[PACKET_MAX_SIZE] = {BYTE_CONTROL,        // Sync
                                                      0x00,                // Packet type
                                                      0x01,0x30,           // Unknown
                                                      0x00,                // Payload Size
                                                      0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Payload
                                                      0x00};

/* A class representing the raw packet sent to or from the Mitsubishi equipment with definitions
for header indexes, checksum calculations, utility methods, etc.  These generally shouldn't be accessed
directly outside the MUARTBridge, and the Packet class (or its subclasses) should be used instead.
*/
class RawPacket {
 public:
  RawPacket(const uint8_t packet_bytes[], const uint8_t packet_length);  // For reading or copying packets
  // TODO: Can I hide this constructor except from optional?
  RawPacket(); // For optional<RawPacket> construction
  RawPacket(PacketType packet_type, uint8_t payload_size);  // For building packets
  virtual ~RawPacket() {}

  virtual std::string to_string() const {return format_hex_pretty(&getBytes()[0], getLength());};

  const uint8_t getLength() const { return length; };
  const uint8_t *getBytes() const { return packetBytes; };  // Primarily for sending packets

  bool isChecksumValid() const;

  // Returns the packet type byte
  uint8_t getPacketType() const { return packetBytes[PACKET_HEADER_INDEX_PACKET_TYPE]; };
  // Returns the first byte of the payload, often used as a command
  uint8_t getCommand() const { return getPayloadByte(PLINDEX_COMMAND); };

  RawPacket &setPayloadByte(const uint8_t payload_byte_index, const uint8_t value);
  uint8_t getPayloadByte(const uint8_t payload_byte_index) const {
      return packetBytes[PACKET_HEADER_SIZE + payload_byte_index];
    };

 private:
  static const int PLINDEX_COMMAND = 0;

  uint8_t packetBytes[PACKET_MAX_SIZE]{};
  uint8_t length;
  uint8_t checksumIndex;

  uint8_t calculateChecksum() const;
  RawPacket &updateChecksum();
};

}  // namespace mitsubishi_uart
}  // namespace esphome
