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

// Used to specify
enum GetCommand : uint8_t {
  gc_settings = 0x02,
  gc_room_temp = 0x03,
  gc_four = 0x04,
  gc_status = 0x06,
  gc_standby = 0x09
};

enum SetCommand : uint8_t {
  sc_settings = 0x01,
  sc_remote_temperature = 0x07
};

static const uint8_t EMPTY_PACKET[PACKET_MAX_SIZE] = {BYTE_CONTROL,        // Sync
                                                      0x00,                // Packet type
                                                      0x01,0x30,           // Unknown
                                                      0x00,                // Payload Size
                                                      0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Payload
                                                      0x00};               // Checksum

class PacketProcessor;

class Packet {
 public:
  Packet(const uint8_t packet_bytes[], const uint8_t packet_length);  // For reading or copying packets
  // TODO: Can I hide this constructor except from optional?
  Packet(); // For optional<Packet> construction
  virtual ~Packet() {}
  const uint8_t getLength() const { return length; };
  const uint8_t *getBytes() const { return packetBytes; };  // Primarily for sending packets

  bool isChecksumValid() const;

  // Returns the packet type byte
  uint8_t getPacketType() const { return packetBytes[PACKET_HEADER_INDEX_PACKET_TYPE]; };
  // Returns the first byte of the payload, often used as a command
  uint8_t getCommand() const { return packetBytes[PACKET_HEADER_SIZE + PLINDEX_COMMAND]; };

  virtual void process(PacketProcessor &pp);
 protected:
  static const int PLINDEX_COMMAND = 0;
  static const int PLINDEX_FLAGS = 1;

  Packet(PacketType packet_type, uint8_t payload_size);  // For building packets

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
class ConnectRequestPacket : public Packet {
 public:
  ConnectRequestPacket() : Packet(PacketType::connect_request, 2) {
    setPayloadByte(0, 0xca);
    setPayloadByte(1, 0x01);
  }
  using Packet::Packet;
};

class ConnectResponsePacket : public Packet {
  using Packet::Packet;
  void process(PacketProcessor &pp) override;
};

////
// Extended Connect
////
class ExtendedConnectRequestPacket : public Packet {
 public:
  ExtendedConnectRequestPacket() : Packet(PacketType::extended_connect_request, 2) {
    setPayloadByte(0, 0xca);
    setPayloadByte(1, 0x01);
  }
  using Packet::Packet;
};

class ExtendedConnectResponsePacket : public Packet {
  using Packet::Packet;
  void process(PacketProcessor &pp) override;
};

////
// Get
////
class GetRequestPacket : public Packet {
 public:
  GetRequestPacket(GetCommand get_command) : Packet(PacketType::get_request, 1) {
    setPayloadByte(0, get_command);
  }
  using Packet::Packet;
};

class SettingsGetResponsePacket : public Packet {
  static const int PLINDEX_POWER = 3;
  static const int PLINDEX_MODE = 4;
  static const int PLINDEX_TARGETTEMP = 11;
  static const int PLINDEX_FAN = 6;
  static const int PLINDEX_VANE = 7;
  static const int PLINDEX_HVANE = 10;
  using Packet::Packet;
  void process(PacketProcessor &pp) override;

 public:
  bool getPower() const { return this->getPayloadByte(PLINDEX_POWER); }
  uint8_t getMode() const { return this->getPayloadByte(PLINDEX_MODE); }
  float getTargetTemp() const { return ((int) this->getPayloadByte(PLINDEX_TARGETTEMP) - 128) / 2.0f; }
  uint8_t getFan() const { return this->getPayloadByte(PLINDEX_FAN); }
  uint8_t getVane() const { return this->getPayloadByte(PLINDEX_VANE); }
  uint8_t getHorizontalVane() const { return this->getPayloadByte(PLINDEX_HVANE); }
};

class RoomTempGetResponsePacket : public Packet {
  static const int PLINDEX_ROOMTEMP_CODE = 3;  // TODO: I don't know why I would use this instead of the one below...
  static const int PLINDEX_ROOMTEMP = 6;
  using Packet::Packet;
  void process(PacketProcessor &pp) override;

 public:
  float getRoomTemp() const { return ((int) this->getPayloadByte(PLINDEX_ROOMTEMP) - 128) / 2.0f; }
};

class StatusGetResponsePacket : public Packet {
  static const int PLINDEX_COMPRESSOR_FREQUENCY = 3;
  static const int PLINDEX_OPERATING = 4;

  using Packet::Packet;
  void process(PacketProcessor &pp) override;

 public:
  uint8_t getCompressorFrequency() const { return this->getPayloadByte(PLINDEX_COMPRESSOR_FREQUENCY); }
  bool getOperating() const { return this->getPayloadByte(PLINDEX_OPERATING); }
};

class StandbyGetResponsePacket : public Packet {
  static const int PLINDEX_LOOPSTATUS = 3;
  static const int PLINDEX_STAGE = 4;
  using Packet::Packet;
  void process(PacketProcessor &pp) override;

 public:
  uint8_t getLoopStatus() const { return this->getPayloadByte(PLINDEX_LOOPSTATUS); }
  uint8_t getStage() const { return this->getPayloadByte(PLINDEX_STAGE); }
};

////
// Set
////

class SettingsSetRequestPacket : public Packet {
  static const int PLINDEX_FLAGS2 = 2;
  static const int PLINDEX_POWER = 3;
  static const int PLINDEX_MODE = 4;
  static const int PLINDEX_TARGET_TEMPERATURE_CODE = 6;
  static const int PLINDEX_FAN = 6;
  static const int PLINDEX_VANE = 7;
  static const int PLINDEX_HORIZONTAL_VANE = 13;
  static const int PLINDEX_TARGET_TEMPERATURE = 14;

  enum SETTING_FLAG : uint8_t {
    SF_POWER = 0x01,
    SF_MODE = 0x02,
    SF_TARGET_TEMPERATURE = 0x04,
    SF_FAN = 0x08,
    SF_VANE = 0x10
  };

  enum SETTING_FLAG2 : uint8_t {
    SF2_HORIZONTAL_VANE = 0x01,
  };

 public:
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

  SettingsSetRequestPacket() : Packet(PacketType::set_request, 16) { setPayloadByte(0, SetCommand::sc_settings); }
  using Packet::Packet;

  SettingsSetRequestPacket &setPower(const bool isOn);
  SettingsSetRequestPacket &setMode(const MODE_BYTE mode);
  SettingsSetRequestPacket &setTargetTemperature(const float temperatureDegressC);
  SettingsSetRequestPacket &setFan(const FAN_BYTE fan);
  SettingsSetRequestPacket &setVane(const VANE_BYTE vane);
  SettingsSetRequestPacket &setHorizontalVane(const HORIZONTAL_VANE_BYTE horizontal_vane);

 private:
  void addFlag(const SETTING_FLAG flagToAdd);
  void addFlag2(const SETTING_FLAG2 flag2ToAdd);
};

class RemoteTemperatureSetRequestPacket : public Packet {
  static const uint8_t PLINDEX_REMOTE_TEMPERATURE = 3;

 public:
  RemoteTemperatureSetRequestPacket() : Packet(PacketType::set_request, 4) {
    setPayloadByte(0, SetCommand::sc_remote_temperature);
  }
  using Packet::Packet;

  RemoteTemperatureSetRequestPacket &setRemoteTemperature(const float temperatureDegressC);
  RemoteTemperatureSetRequestPacket &useInternalTemperature();

  uint8_t getFlags() const { return getPayloadByte(PLINDEX_FLAGS); }
  float getRemoteTemperature() const { return ((int) getPayloadByte(PLINDEX_REMOTE_TEMPERATURE) - 128) / 2.0f; }
};

class RemoteTemperatureSetResponsePacket : public Packet {
 public:
  RemoteTemperatureSetResponsePacket() : Packet(PacketType::set_response, 16) {}
  using Packet::Packet;
  void process(PacketProcessor &pp) override;
};

class PacketProcessor {
  public:
    virtual void processGenericPacket(const Packet &packet) {};
    virtual void processConnectResponsePacket(const ConnectResponsePacket &packet) {};
    virtual void processExtendedConnectResponsePacket(const ExtendedConnectResponsePacket &packet) {};
    virtual void processSettingsGetResponsePacket(const SettingsGetResponsePacket &packet) {};
    virtual void processRoomTempGetResponsePacket(const RoomTempGetResponsePacket &packet) {};
    virtual void processStatusGetResponsePacket(const StatusGetResponsePacket &packet) {};
    virtual void processStandbyGetResponsePacket(const StandbyGetResponsePacket &packet) {};
    virtual void processRemoteTemperatureSetResponsePacket(const RemoteTemperatureSetResponsePacket &packet) {};

};

}  // namespace mitsubishi_uart
}  // namespace esphome
