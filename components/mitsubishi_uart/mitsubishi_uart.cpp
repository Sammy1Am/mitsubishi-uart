#include "mitsubishi_uart.h"

namespace esphome {
namespace mitsubishi_uart {

////
// Structure Comparison
// Allows for quick state comparisons (this might get unwieldy eventually)
////
bool operator!=(const muartState &lhs, const muartState &rhs) {
  return lhs.c_action != rhs.c_action || lhs.c_current_temperature != rhs.c_current_temperature ||
         lhs.c_fan_mode != rhs.c_fan_mode || lhs.c_mode != rhs.c_mode ||
         lhs.c_target_temperature != rhs.c_target_temperature;
}

// TODO (Can I move these into the packets files?)
// Pre-built packets
const Packet PACKET_CONNECT_REQ = PacketConnectRequest();
const Packet PACKET_SETTINGS_REQ = PacketGetRequest(PacketGetCommand::settings);
const Packet PACKET_TEMP_REQ = PacketGetRequest(PacketGetCommand::room_temp);
const Packet PACKET_STATUS_REQ = PacketGetRequest(PacketGetCommand::status);
const Packet PACKET_STANDBY_REQ = PacketGetRequest(PacketGetCommand::standby);

////
// MitsubishiUART
////

MitsubishiUART::MitsubishiUART(uart::UARTComponent *uart_comp) : hp_uart{uart_comp} {
  this->_traits.set_supports_action(true);
  this->_traits.set_supports_current_temperature(true);
  this->_traits.set_supports_two_point_target_temperature(false);
  this->_traits.set_visual_min_temperature(MUART_MIN_TEMP);
  this->_traits.set_visual_max_temperature(MUART_MAX_TEMP);
  this->_traits.set_visual_temperature_step(MUART_TEMPERATURE_STEP);
}

void MitsubishiUART::setup() {
  this->restore_state_();
  connect();
}

climate::ClimateTraits MitsubishiUART::traits() { return _traits; }
climate::ClimateTraits &MitsubishiUART::config_traits() { return _traits; }

void MitsubishiUART::control(const climate::ClimateCall &call) {}

void MitsubishiUART::update() {
  ESP_LOGV(TAG, "Update called.");

  int packetsRead = 0;
  packetsRead += readPacket(false);  // Check for connection results or other residual packets, but don't wait for them

  /**
   * For whatever reason:
   * - Sending multiple request packets very quickly will result in only a response to the first one.  It seems
   * like the heat pump may not be entirely breaking communications by control byte (or possibly it empties the
   * input buffer between requests?)
   *
   * - Sending multiple request packets sort of quickly results in multiple responses, but the first responses
   * are missing their checksums.
   *
   * As a result, we blocking-read for a response as part of sendPacket()
   */
  if (connectState == 2) {
    // Request room temp
    sendPacket(PACKET_TEMP_REQ) ? packetsRead++ : 0;
    // Request status
    sendPacket(PACKET_STATUS_REQ) ? packetsRead++ : 0;
    // Request settings
    sendPacket(PACKET_SETTINGS_REQ) ? packetsRead++ : 0;
    //
    sendPacket(PACKET_STANDBY_REQ) ? packetsRead++ : 0;
  }

  if (packetsRead > 0) {
    updatesSinceLastPacket = 0;
  } else {
    updatesSinceLastPacket++;
  }

  if (updatesSinceLastPacket > 10) {
    ESP_LOGI(TAG, "No packets received in %d updates, connection down.", updatesSinceLastPacket);
    connectState = 0;
  }

  muartState currentState = getCurrentState();

  if (currentState != lastPublishedState) {
    this->publish_state();
    lastPublishedState = currentState;
  }

  // If we're not connected (or have become unconnected) try to send a connect packet again
  if (connectState < 2) {
    connect();
  }
}

muartState MitsubishiUART::getCurrentState() {
  muartState currentState{};
  currentState.c_action = this->action;
  currentState.c_current_temperature = this->current_temperature;
  currentState.c_fan_mode = this->fan_mode.value_or(climate::ClimateFanMode::CLIMATE_FAN_OFF);
  currentState.c_mode = this->mode;
  currentState.c_target_temperature = this->target_temperature;
  return currentState;
}

void MitsubishiUART::dump_config() {
  ESP_LOGCONFIG(TAG, "Mitsubishi UART v%s", MUART_VERSION);
  ESP_LOGCONFIG(TAG, "Connection state: %d", connectState);
}

void MitsubishiUART::connect() {
  connectState = 1;  // Connecting...
  sendPacket(PACKET_CONNECT_REQ);
}

bool MitsubishiUART::sendPacket(Packet packet, bool expectResponse) {
  hp_uart->write_array(packet.getBytes(), packet.getLength());
  if (expectResponse) {
    return readPacket();
  }
  return false;
}

/**
 * Reads packets from UART, and sends them to appropriate handler methods.
 */
bool MitsubishiUART::readPacket(bool waitForPacket) {
  uint8_t p_byte;
  bool foundPacket = false;
  unsigned long readStop = millis() + PACKET_RECEIVE_TIMEOUT;

  while (millis() < readStop) {
    // Search for control byte (or check that one is waiting for us)
    while (hp_uart->available() > PACKET_HEADER_SIZE && hp_uart->peek_byte(&p_byte)) {
      if (p_byte == BYTE_CONTROL) {
        foundPacket = true;
        ESP_LOGV(TAG, "FoundPacket!");
        break;
      } else {
        hp_uart->read_byte(&p_byte);
      }
    }
    if (foundPacket) {
      break;
    }
    if (!waitForPacket) {
      break;
    }
    delay(10);
  }

  // If control byte has been found and there's at least a header available, parse the packet
  if (foundPacket && hp_uart->available() > PACKET_HEADER_SIZE) {
    uint8_t p_header[PACKET_HEADER_SIZE];
    hp_uart->read_array(p_header, PACKET_HEADER_SIZE);
    int payloadSize = p_header[PACKET_HEADER_INDEX_PAYLOAD_SIZE];
    uint8_t p_payload[payloadSize];
    uint8_t checksum;
    hp_uart->read_array(p_payload, payloadSize);
    hp_uart->read_byte(&checksum);

    switch (p_header[PACKET_HEADER_INDEX_PACKET_TYPE]) {
      case PacketType::connect_response:
        hResConnect(PacketConnectResponse(p_header, p_payload, payloadSize, checksum));
        break;
      case PacketType::get_response:
        switch (p_payload[0]) {  // Switch on command type
          case PacketGetCommand::settings:
            hResGetSettings(PacketGetResponseSettings(p_header, p_payload, payloadSize, checksum));
            break;
          case PacketGetCommand::room_temp:
            hResGetRoomTemp(PacketGetResponseRoomTemp(p_header, p_payload, payloadSize, checksum));
            break;
          case PacketGetCommand::status:
            hResGetStatus(PacketGetResponseStatus(p_header, p_payload, payloadSize, checksum));
            break;
          case PacketGetCommand::standby:
            hResGetStandby(PacketGetResponseStandby(p_header, p_payload, payloadSize, checksum));
            break;
          default:
            ESP_LOGI(TAG, "Unknown get response command %x received.", p_payload[0]);
        }
        break;
      default:
        ESP_LOGI(TAG, "Unknown packet type %x received.", p_header[PACKET_HEADER_INDEX_PACKET_TYPE]);
    }

    return true;
  }

  return false;
}

void MitsubishiUART::hResConnect(PacketConnectResponse packet) {
  // Not sure there's any info in the response.
  connectState = 2;
  ESP_LOGI(TAG, "Connected to heatpump.");
}

void MitsubishiUART::hResGetSettings(PacketGetResponseSettings packet) {
  bool power = packet.getPower();
  if (power) {
    switch (packet.getMode()) {
      case 0x01:
        this->mode = climate::CLIMATE_MODE_HEAT;
        break;
      case 0x02:
        this->mode = climate::CLIMATE_MODE_DRY;
        break;
      case 0x03:
        this->mode = climate::CLIMATE_MODE_COOL;
        break;
      case 0x07:
        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
        break;
      case 0x08:
        this->mode = climate::CLIMATE_MODE_HEAT_COOL;
        break;
      default:
        this->mode = climate::CLIMATE_MODE_OFF;
    }
  } else {
    this->mode = climate::CLIMATE_MODE_OFF;
  }

  this->target_temperature = packet.getTargetTemp();

  switch (packet.getFan()) {
    case 0x00:
      this->fan_mode = climate::CLIMATE_FAN_AUTO;
      break;
    case 0x01:
      this->fan_mode = climate::CLIMATE_FAN_QUIET;
      break;
    case 0x02:
      this->fan_mode = climate::CLIMATE_FAN_LOW;
      break;
    case 0x03:
      this->fan_mode = climate::CLIMATE_FAN_MIDDLE;
      break;
    case 0x05:
      this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
      break;
    case 0x06:
      this->fan_mode = climate::CLIMATE_FAN_HIGH;
      break;
  }

  uint8_t vane = packet.getVane();
  if (vane>0x05) {vane = 0x06;} // "Swing" is 0x07 and there's no 0x06, so the select menu index only goes to 6
  this->select_vane_direction->updateIndex(vane);

  uint8_t h_vane = packet.getHorizontalVane();
  ESP_LOGD(TAG, "HVane set to: %x", h_vane);
}

void MitsubishiUART::hResGetRoomTemp(PacketGetResponseRoomTemp packet) {
  this->current_temperature = packet.getRoomTemp();
  ESP_LOGD(TAG, "Room temp: %.1f", this->current_temperature);
}

void MitsubishiUART::hResGetStatus(PacketGetResponseStatus packet) {
  bool operating = packet.getOperating();
  uint8_t comressporFreq = packet.getCompressorFrequency();

  // TODO Simplify this switch (too many redundant ACTION_IDLES)
  switch (this->mode) {
    case climate::CLIMATE_MODE_HEAT:
      if (operating) {
        this->action = climate::CLIMATE_ACTION_HEATING;
      } else {
        this->action = climate::CLIMATE_ACTION_IDLE;
      }
      break;
    case climate::CLIMATE_MODE_COOL:
      if (operating) {
        this->action = climate::CLIMATE_ACTION_COOLING;
      } else {
        this->action = climate::CLIMATE_ACTION_IDLE;
      }
      break;
    case climate::CLIMATE_MODE_HEAT_COOL:
      this->action = climate::CLIMATE_ACTION_IDLE;
      if (operating) {
        if (this->current_temperature > this->target_temperature) {
          this->action = climate::CLIMATE_ACTION_COOLING;
        } else if (this->current_temperature < this->target_temperature) {
          this->action = climate::CLIMATE_ACTION_HEATING;
        }
      }
      break;
    case climate::CLIMATE_MODE_DRY:
      if (operating) {
        this->action = climate::CLIMATE_ACTION_DRYING;
      } else {
        this->action = climate::CLIMATE_ACTION_IDLE;
      }
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      this->action = climate::CLIMATE_ACTION_FAN;
      break;
    default:
      this->action = climate::CLIMATE_ACTION_OFF;
  }

  ESP_LOGD(TAG, "Operating: %s Frequency: %d", YESNO(operating), comressporFreq);
}
void MitsubishiUART::hResGetStandby(PacketGetResponseStandby packet) {
  // TODO these are a little uncertain
  // 0x04 = pre-heat, 0x08 = standby
  uint8_t loopStatus = packet.getLoopStatus();
  // 1 to 5, lowest to highest power
  uint8_t stage = packet.getStage();
  ESP_LOGD(TAG, "Loop status: %x Stage: %x", loopStatus, stage);
}

}  // namespace mitsubishi_uart
}  // namespace esphome
