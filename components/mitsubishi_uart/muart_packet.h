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
const uint8_t PACKET_HEADER_INDEX_PAYLOAD_SIZE = 4;

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

enum PacketGetCommand : uint8_t {
  gc_settings = 0x02,
  gc_room_temp = 0x03,
  gc_four = 0x04,
  gc_status = 0x06,
  gc_standby = 0x09
};

enum PacketSetCommand : uint8_t { sc_settings = 0x01, sc_remote_temperature = 0x07 };

static const uint8_t EMPTY_PACKET[PACKET_MAX_SIZE] = {BYTE_CONTROL,        // Sync
                                                      0x00,                // Packet type
                                                      0x01,         0x30,  // Unknown
                                                      0x00,                // Payload Size
                                                      0x00,         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                      0x00,         0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Payload
                                                      0x00};                                             // Checksum

class Packet {
 public:
  Packet(const uint8_t packet_bytes[], const uint8_t packet_length);  // For reading or copying packets
  virtual ~Packet() {}
  const uint8_t *getBytes() const { return packetBytes; };  // Primarily for sending packets
  const uint8_t getLength() const { return length; };

  // Did this packet originate outside our microcontroller (e.g. from a thermostat); used
  // to determine if packet should be forwarded or not.
  bool isExternal = false;

  bool isChecksumValid() const;

  // Returns the packet type byte
  uint8_t getPacketType() const { return packetBytes[PACKET_HEADER_INDEX_PACKET_TYPE]; };
  // Returns the first byte of the payload, often used as a command
  uint8_t getCommand() const { return packetBytes[PACKET_HEADER_SIZE + PAYLOAD_INDEX_COMMAND]; };

 protected:
  static const int PAYLOAD_INDEX_COMMAND = 0;
  static const int PAYLOAD_INDEX_FLAGS = 1;

  Packet(uint8_t packet_type, uint8_t payload_size);  // For building packets

  Packet &setPayloadByte(uint8_t payload_byte_index, uint8_t value);
  uint8_t getPayloadByte(uint8_t payload_byte_index) const {
    return packetBytes[PACKET_HEADER_SIZE + payload_byte_index];
  };

 private:
  uint8_t length;
  uint8_t checksumIndex;
  uint8_t packetBytes[PACKET_MAX_SIZE]{};
  uint8_t calculateChecksum() const;
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
  using Packet::Packet;
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
  bool getPower() const { return this->getBytes()[INDEX_POWER]; }
  uint8_t getMode() const { return this->getBytes()[INDEX_MODE]; }
  float getTargetTemp() const { return ((int) this->getBytes()[INDEX_TARGETTEMP] - 128) / 2.0f; }
  uint8_t getFan() const { return this->getBytes()[INDEX_FAN]; }
  uint8_t getVane() const { return this->getBytes()[INDEX_VANE]; }
  uint8_t getHorizontalVane() const { return this->getBytes()[INDEX_HVANE]; }
};

class PacketGetResponseRoomTemp : public Packet {
  static const int INDEX_ROOMTEMP_CODE = 8;  // TODO: I don't know why I would use this instead of the one below...
  static const int INDEX_ROOMTEMP = 11;
  using Packet::Packet;

 public:
  float getRoomTemp() const { return ((int) this->getBytes()[INDEX_ROOMTEMP] - 128) / 2.0f; }
};

class PacketGetResponseStatus : public Packet {
  static const int INDEX_OPERATING = 9;
  static const int INDEX_COMPRESSOR_FREQUENCY = 8;
  using Packet::Packet;

 public:
  bool getOperating() const { return this->getBytes()[INDEX_OPERATING]; }
  uint8_t getCompressorFrequency() const { return this->getBytes()[INDEX_COMPRESSOR_FREQUENCY]; }
};

class PacketGetResponseStandby : public Packet {
  static const int INDEX_LOOPSTATUS = 8;
  static const int INDEX_STAGE = 9;
  using Packet::Packet;

 public:
  uint8_t getLoopStatus() const { return this->getBytes()[INDEX_LOOPSTATUS]; }
  uint8_t getStage() const { return this->getBytes()[INDEX_STAGE]; }
};

////
// Set
////

class PacketSetSettingsRequest : public Packet {
  static const int PAYLOAD_INDEX_POWER = 3;
  static const int PAYLOAD_INDEX_MODE = 4;
  static const int PAYLOAD_INDEX_TARGET_TEMPERATURE_CODE = 6;
  static const int PAYLOAD_INDEX_FAN = 6;
  static const int PAYLOAD_INDEX_VANE = 7;
  static const int PAYLOAD_INDEX_HORIZONTAL_VANE = 13;
  static const int PAYLOAD_INDEX_TARGET_TEMPERATURE = 14;

  enum SETTING_FLAG : uint8_t {
    SF_POWER = 0x01,
    SF_MODE = 0x02,
    SF_TARGET_TEMPERATURE = 0x04,
    SF_FAN = 0x08,
    SF_VANE = 0x10
  };

  enum MODE_BYTE : uint8_t {
    MODE_BYTE_HEAT = 0x01,
    MODE_BYTE_DRY = 0x02,
    MODE_BYTE_COOL = 0x03,
    MODE_BYTE_FAN = 0x07,
    MODE_BYTE_AUTO = 0x08,
  };

  enum FAN_BYTE : uint8_t {
    FAN_AUTO = 0x00,
    FAN_QUIET = 0x01,
    FAN_1 = 0x02,
    FAN_2 = 0x03,
    FAN_3 = 0x05,
    FAN_4 = 0x06,
  };

  enum VANE_BYTE : uint8_t {
    VANE_AUTO = 0x00,
    VANE_1 = 0x01,
    VANE_2 = 0x02,
    VANE_3 = 0x03,
    VANE_4 = 0x04,
    VANE_5 = 0x05,
    VANE_SWING = 0x07,
  };

  enum HORIZONTAL_VANE_BYTE : uint8_t {
    HV_LEFT_FULL = 0x01,
    HV_LEFT = 0x02,
    HV_CENTER = 0x03,
    HV_RIGHT = 0x04,
    HV_RIGHT_FULL = 0x05,
    HV_SPLIT = 0x08,
    HV_SWING = 0x0c,
  };

 public:
  PacketSetSettingsRequest() : Packet(PacketType::set_request, 16) { setPayloadByte(0, PacketSetCommand::sc_settings); }
  using Packet::Packet;

  PacketSetSettingsRequest &setPower(const bool isOn);
  PacketSetSettingsRequest &setMode(const MODE_BYTE mode);
  PacketSetSettingsRequest &setTargetTemperature(const float temperatureDegressC);
  PacketSetSettingsRequest &setFan(const FAN_BYTE fan);
  PacketSetSettingsRequest &setVane(const VANE_BYTE vane);
  PacketSetSettingsRequest &setHorizontalVane(const HORIZONTAL_VANE_BYTE horizontal_vane);

 private:
  void addFlag(const SETTING_FLAG flagToAdd);
};

class PacketSetRemoteTemperatureRequest : public Packet {
  static const uint8_t PAYLOAD_INDEX_REMOTE_TEMPERATURE = 3;

 public:
  PacketSetRemoteTemperatureRequest() : Packet(PacketType::set_request, 4) {
    setPayloadByte(0, PacketSetCommand::sc_remote_temperature);
  }
  using Packet::Packet;

  PacketSetRemoteTemperatureRequest &setRemoteTemperature(const float temperatureDegressC);
  PacketSetRemoteTemperatureRequest &useInternalTemperature();

  float getRemoteTemperature() const { return ((int) getPayloadByte(PAYLOAD_INDEX_REMOTE_TEMPERATURE) - 128) / 2.0f; }
};

}  // namespace mitsubishi_uart
}  // namespace esphome
