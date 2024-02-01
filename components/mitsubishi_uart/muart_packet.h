#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"
#include "muart_rawpacket.h"
#include <sstream>

namespace esphome {
namespace mitsubishi_uart {

#define LOGPACKET(packet, direction) ESP_LOGD("mitsubishi_uart.packets", "%s [%02x] %s", direction, packet.getPacketType(), packet.to_string().c_str());

#define CONSOLE_COLOR_NONE "\033[0m"
#define CONSOLE_COLOR_GREEN "\033[0;32m"
#define CONSOLE_COLOR_PURPLE "\033[0;35m"
#define CONSOLE_COLOR_CYAN "\033[0;36m"
#define CONSOLE_COLOR_CYAN_BOLD "\033[1;36m"
#define CONSOLE_COLOR_WHITE "\033[0;37m"



class PacketProcessor;

// Generic Base Packet wrapper over RawPacket
class Packet {
  public:
    Packet(RawPacket&& pkt) : pkt_(std::move(pkt)) {}; // TODO: Confirm this needs std::move if call to constructor ALSO has move
    Packet(); // For optional<> construction

    // Returns a (more) human readable string of the packet
    virtual std::string to_string() const;

    // Is a response packet expected when this packet is sent.  Defaults to true since
    // most requests receive a response.
    const bool isResponseExpected() const {return responseExpected;};

    // Passthrough methods to RawPacket
    RawPacket& rawPacket() {return pkt_;};
    uint8_t getPacketType() const {return pkt_.getPacketType();}
    bool isChecksumValid() const {return pkt_.isChecksumValid();};

    // Returns flags (ONLY APPLICABLE FOR SOME COMMANDS)
    uint8_t getFlags() const {return pkt_.getPayloadByte(PLINDEX_FLAGS);}
    // Sets flags (ONLY APPLICABLE FOR SOME COMMANDS)
    void setFlags(const uint8_t flagValue);
    // Adds a flag (ONLY APPLICABLE FOR SOME COMMANDS)
    void addFlag(const uint8_t flagToAdd);
    // Adds a flag2 (ONLY APPLICABLE FOR SOME COMMANDS)
    void addFlag2(const uint8_t flag2ToAdd);

  protected:
    static const int PLINDEX_FLAGS = 1;
    static const int PLINDEX_FLAGS2 = 2;

    RawPacket pkt_;
  private:
    bool responseExpected = true;
};

////
// Connect
////
class ConnectRequestPacket : public Packet {
 public:

  static ConnectRequestPacket& instance() {
    static ConnectRequestPacket INSTANCE;
    return INSTANCE;
  }

  std::string to_string() const override;
 private:
  ConnectRequestPacket() : Packet(RawPacket(PacketType::connect_request, 2)) {
    pkt_.setPayloadByte(0, 0xca);
    pkt_.setPayloadByte(1, 0x01);
  }
};

class ConnectResponsePacket : public Packet {

  public:
    using Packet::Packet;
    std::string to_string() const override;
};

////
// Extended Connect
////
class ExtendedConnectRequestPacket : public Packet {
 public:
  static ExtendedConnectRequestPacket& instance() {
    static ExtendedConnectRequestPacket INSTANCE;
    return INSTANCE;
  }
  using Packet::Packet;
 private:
  ExtendedConnectRequestPacket() : Packet(RawPacket(PacketType::extended_connect_request, 2)) {
    pkt_.setPayloadByte(0, 0xca);
    pkt_.setPayloadByte(1, 0x01);
  }
};

class ExtendedConnectResponsePacket : public Packet {
  using Packet::Packet;
};

////
// Get
////
class GetRequestPacket : public Packet {
 public:
  static GetRequestPacket& getSettingsInstance() {
    static GetRequestPacket INSTANCE = GetRequestPacket(GetCommand::gc_settings);
    return INSTANCE;
  }
  static GetRequestPacket& getCurrentTempInstance() {
    static GetRequestPacket INSTANCE = GetRequestPacket(GetCommand::gc_current_temp);
    return INSTANCE;
  }
  static GetRequestPacket& getStatusInstance() {
    static GetRequestPacket INSTANCE = GetRequestPacket(GetCommand::gc_standby);
    return INSTANCE;
  }
  static GetRequestPacket& getStandbyInstance() {
    static GetRequestPacket INSTANCE = GetRequestPacket(GetCommand::gc_status);
    return INSTANCE;
  }
  using Packet::Packet;

 private:
  GetRequestPacket(GetCommand get_command) : Packet(RawPacket(PacketType::get_request, 1)) {
    pkt_.setPayloadByte(0, get_command);
  }
};

class SettingsGetResponsePacket : public Packet {
  static const int PLINDEX_POWER = 3;
  static const int PLINDEX_MODE = 4;
  static const int PLINDEX_TARGETTEMP = 11;
  static const int PLINDEX_FAN = 6;
  static const int PLINDEX_VANE = 7;
  static const int PLINDEX_HVANE = 10;
  using Packet::Packet;

 public:
  bool getPower() const { return pkt_.getPayloadByte(PLINDEX_POWER); }
  uint8_t getMode() const { return pkt_.getPayloadByte(PLINDEX_MODE); }
  float getTargetTemp() const { return ((int) pkt_.getPayloadByte(PLINDEX_TARGETTEMP) - 128) / 2.0f; }
  uint8_t getFan() const { return pkt_.getPayloadByte(PLINDEX_FAN); }
  uint8_t getVane() const { return pkt_.getPayloadByte(PLINDEX_VANE); }
  uint8_t getHorizontalVane() const { return pkt_.getPayloadByte(PLINDEX_HVANE); }

  std::string to_string() const override;
};

class CurrentTempGetResponsePacket : public Packet {
  static const int PLINDEX_CURRENTTEMP_CODE = 3;  // TODO: I don't know why I would use this instead of the one below...
  static const int PLINDEX_CURRENTTEMP = 6;
  using Packet::Packet;

 public:
  float getCurrentTemp() const { return ((int) pkt_.getPayloadByte(PLINDEX_CURRENTTEMP) - 128) / 2.0f; }
  std::string to_string() const override;
};

class StatusGetResponsePacket : public Packet {
  static const int PLINDEX_COMPRESSOR_FREQUENCY = 3;
  static const int PLINDEX_OPERATING = 4;

  using Packet::Packet;


 public:
  uint8_t getCompressorFrequency() const { return pkt_.getPayloadByte(PLINDEX_COMPRESSOR_FREQUENCY); }
  bool getOperating() const { return pkt_.getPayloadByte(PLINDEX_OPERATING); }
  std::string to_string() const override;
};

class StandbyGetResponsePacket : public Packet {
  static const int PLINDEX_LOOPSTATUS = 3;
  static const int PLINDEX_STAGE = 4;
  using Packet::Packet;

 public:
  uint8_t getLoopStatus() const { return pkt_.getPayloadByte(PLINDEX_LOOPSTATUS); }
  uint8_t getStage() const { return pkt_.getPayloadByte(PLINDEX_STAGE); }
  std::string to_string() const override;
};

////
// Set
////

class SettingsSetRequestPacket : public Packet {
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

  SettingsSetRequestPacket() : Packet(RawPacket(PacketType::set_request, 16)) { pkt_.setPayloadByte(0, SetCommand::sc_settings); }
  using Packet::Packet;

  SettingsSetRequestPacket &setPower(const bool isOn);
  SettingsSetRequestPacket &setMode(const MODE_BYTE mode);
  SettingsSetRequestPacket &setTargetTemperature(const float temperatureDegressC);
  SettingsSetRequestPacket &setFan(const FAN_BYTE fan);
  SettingsSetRequestPacket &setVane(const VANE_BYTE vane);
  SettingsSetRequestPacket &setHorizontalVane(const HORIZONTAL_VANE_BYTE horizontal_vane);

 private:
  void addSettingsFlag(const SETTING_FLAG flagToAdd);
  void addSettingsFlag2(const SETTING_FLAG2 flag2ToAdd);
};

class RemoteTemperatureSetRequestPacket : public Packet {
  static const uint8_t PLINDEX_REMOTE_TEMPERATURE = 3;

 public:
  RemoteTemperatureSetRequestPacket() : Packet(RawPacket(PacketType::set_request, 4)) {
    pkt_.setPayloadByte(0, SetCommand::sc_remote_temperature);
  }
  using Packet::Packet;

  RemoteTemperatureSetRequestPacket &setRemoteTemperature(const float temperatureDegressC);
  RemoteTemperatureSetRequestPacket &useInternalTemperature();

  float getRemoteTemperature() const { return ((int) pkt_.getPayloadByte(PLINDEX_REMOTE_TEMPERATURE) - 128) / 2.0f; }
};

class RemoteTemperatureSetResponsePacket : public Packet {
  using Packet::Packet;
 public:
  RemoteTemperatureSetResponsePacket() : Packet(RawPacket(PacketType::set_response, 16)) {}
};

class PacketProcessor {
  public:
    virtual void processGenericPacket(const Packet &packet) {};
    virtual void processConnectResponsePacket(const ConnectResponsePacket &packet) {};
    virtual void processExtendedConnectResponsePacket(const ExtendedConnectResponsePacket &packet) {};
    virtual void processSettingsGetResponsePacket(const SettingsGetResponsePacket &packet) {};
    virtual void processCurrentTempGetResponsePacket(const CurrentTempGetResponsePacket &packet) {};
    virtual void processStatusGetResponsePacket(const StatusGetResponsePacket &packet) {};
    virtual void processStandbyGetResponsePacket(const StandbyGetResponsePacket &packet) {};
    virtual void processRemoteTemperatureSetResponsePacket(const RemoteTemperatureSetResponsePacket &packet) {};

};

}  // namespace mitsubishi_uart
}  // namespace esphome
