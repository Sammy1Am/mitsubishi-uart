#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace mitsubishi_uart {

static const char *PTAG = "mitsubishi_uart";

const uint8_t BYTE_CONTROL = 0xfc;
const int PACKET_MAX_SIZE = 22;  // Used to intialize empty packet
const int PACKET_HEADER_SIZE = 5;
const int PACKET_HEADER_INDEX_PACKET_TYPE = 1;
const int PACKET_HEADER_INDEX_PAYLOAD_SIZE = 4;

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

enum PacketGetCommand : uint8_t { settings = 0x02, room_temp = 0x03, status = 0x06, standby = 0x09 };

static const uint8_t EMPTY_PACKET[PACKET_MAX_SIZE] = {BYTE_CONTROL,        // Sync
                                                      0x00,                // Packet type
                                                      0x01,         0x30,  // Unknown
                                                      0x00,                // Payload Size
                                                      0x00,         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                      0x00,         0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Payload
                                                      0x00};                                             // Checksum

class Packet {
  static const int PAYLOAD_INDEX_COMMAND = 5;

 public:
  // Constructors
  Packet(uint8_t packet_header[PACKET_HEADER_SIZE], uint8_t payload[], uint8_t payload_size,
         uint8_t checksum);                           // For reading packets
  const uint8_t *getBytes() { return packetBytes; };  // Primarily for sending packets
  const int getLength() { return length; };

  const bool isChecksumValid();

  // Packet information getters
  const uint8_t getPacketType() { return packetBytes[PACKET_HEADER_INDEX_PACKET_TYPE]; };
  const uint8_t getCommand() { return packetBytes[PAYLOAD_INDEX_COMMAND]; };

 protected:
  Packet(uint8_t packet_type, uint8_t payload_size);  // For building packets
  Packet &setPayloadByte(int payload_byte_index, uint8_t value);

 private:
  const int length;
  const int checksumIndex;
  uint8_t packetBytes[PACKET_MAX_SIZE]{};
  const uint8_t calculateChecksum();
  Packet &updateChecksum();
};

// TODO Maybe some kind of validation based on packet type in these constructors

////
// Connect
////
class PacketConnectRequest : public Packet {
 public:
  PacketConnectRequest() : Packet(PacketType::connect_request, 2) {
    setPayloadByte(0, 0xca);
    setPayloadByte(1, 0x01);
  }
  using Packet::Packet;
};

class PacketConnectResponse : public Packet {
  using Packet::Packet;
};

////
// Extended Connect
////
class PacketExtendedConnectRequest : public Packet {
 public:
  PacketExtendedConnectRequest() : Packet(PacketType::extended_connect_request, 2) {
    setPayloadByte(0, 0xca);
    setPayloadByte(1, 0x01);
  }
  using Packet::Packet;
};

class PacketExtendedConnectResponse : public Packet {
  using Packet::Packet;
};

////
// Get
////
class PacketGetRequest : public Packet {
 public:
  PacketGetRequest(PacketGetCommand get_command) : Packet(PacketType::get_request, 1) {
    setPayloadByte(0, get_command);
  }
};

class PacketGetResponseSettings : public Packet {
  static const int INDEX_POWER = 8;
  static const int INDEX_MODE = 9;
  static const int INDEX_TARGETTEMP = 16;
  static const int INDEX_FAN = 11;
  static const int INDEX_VANE = 12;
  static const int INDEX_HVANE = 15;
  using Packet::Packet;

 public:
  bool getPower() { return this->getBytes()[INDEX_POWER]; }
  uint8_t getMode() { return this->getBytes()[INDEX_MODE]; }
  float getTargetTemp() { return ((int) this->getBytes()[INDEX_TARGETTEMP] - 128) / 2.0f; }
  uint8_t getFan() { return this->getBytes()[INDEX_FAN]; }
  uint8_t getVane() { return this->getBytes()[INDEX_VANE]; }
  uint8_t getHorizontalVane() { return this->getBytes()[INDEX_HVANE]; }
};

class PacketGetResponseRoomTemp : public Packet {
  static const int INDEX_ROOMTEMP_CODE = 8;  // TODO: I don't know why I would use this instead of the one below...
  static const int INDEX_ROOMTEMP = 11;
  using Packet::Packet;

 public:
  float getRoomTemp() { return ((int) this->getBytes()[INDEX_ROOMTEMP] - 128) / 2.0f; }
};

class PacketGetResponseStatus : public Packet {
  static const int INDEX_OPERATING = 9;
  static const int INDEX_COMPRESSOR_FREQUENCY = 8;
  using Packet::Packet;

 public:
  bool getOperating() { return this->getBytes()[INDEX_OPERATING]; }
  uint8_t getCompressorFrequency() { return this->getBytes()[INDEX_COMPRESSOR_FREQUENCY]; }
};

class PacketGetResponseStandby : public Packet {
  static const int INDEX_LOOPSTATUS = 8;
  static const int INDEX_STAGE = 9;
  using Packet::Packet;

 public:
  uint8_t getLoopStatus() { return this->getBytes()[INDEX_LOOPSTATUS]; }
  uint8_t getStage() { return this->getBytes()[INDEX_STAGE]; }
};

////
// Set
////
// class PacketSetRequest : public Packet {
//   public:
//     PacketSetRequest() : Packet(PKTTYPE, //TODO) {
//     }
// };

// class PacketConnectResponse : public Packet {
//   using Packet::Packet;
// };

////
// Extended Connect
////
// class PacketExtendedConnectRequest : public Packet {
//   static const uint8_t PKTTYPE = 0x5b;
//   public:
//     PacketExtendedConnectRequest() : Packet(PKTTYPE, //TODO) {
//       setPayloadByte(0,0xca);
//       setPayloadByte(1,0x01);
//     }
// };

// class PacketExtendedConnectResponse : public Packet {
//   static const uint8_t PKTTYPE = 0x5a;
//   using Packet::Packet;
// };

}  // namespace mitsubishi_uart
}  // namespace esphome
