#include "mitsubishi_uart.h"

namespace esphome {
namespace mitsubishi_uart {

// TODO (Can I move these into the packets files?)
// Pre-built packets
const Packet PACKET_CONNECT_REQ = PacketConnectRequest();
const Packet PACKET_SETTINGS_REQ = PacketGetRequest(PacketGetCommand::gc_settings);
const Packet PACKET_TEMP_REQ = PacketGetRequest(PacketGetCommand::gc_room_temp);
const Packet PACKET_STATUS_REQ = PacketGetRequest(PacketGetCommand::gc_status);
const Packet PACKET_STANDBY_REQ = PacketGetRequest(PacketGetCommand::gc_standby);

#define DIR_MC_HP "MC->HP"
#define DIR_HP_MC "MC<-HP"
#define DIR_TS_MC "TS->MC"
#define DIR_MC_TS "TS<-MC"

////
// MitsubishiUART
////

MitsubishiUART::MitsubishiUART(uart::UARTComponent *uart_comp) : hp_uart{uart_comp} {}

/**
 * UART communications (especially software UART) seem very sensitive to long loops (and we know
 * WiFi is as well), so the goal in the loop here is to perform one smaller operation at a time.
 * This means using an if-else tree to ensure only one action is performed per loop.
 */
void MitsubishiUART::loop() {
  // Regardless of what we're doing, if there's a queued packet to send to the thermostat, send it
  if (!this->ts_queue_.empty()) {
    // If there are any packets in the ts_queue_
    sendPacket(ts_queue_.front(), tstat_uart);
    ts_queue_.pop_front();
  }

  // If we're idle...
  else if (this->current_loop_state == LS_IDLE) {
    // Try reading from the thermostat (to give it priority)
    if (!(tstat_uart && readPacket(tstat_uart))) {
      // But if we didn't, see if there are any heatpump packets to send
      if (!this->hp_queue_.empty()) {
        // If there are any packets in the ts_queue_
        sendPacket(hp_queue_.front(), hp_uart);
        // Set the current loop state based on if we're waiting on a response for a thermostat packet or
        // heatpump packet
        this->current_loop_state = hp_queue_.front().isExternal ? LS_AWAIT_THERMOSTAT_RESPONSE : LS_AWAIT_MC_RESPONSE;
        this->loop_state_start = millis();
        hp_queue_.pop_front();
      }
    }
  }

  // If we've been waiting too long for a response from the heatpump, go back to idle
  else if (millis() - loop_state_start > LOOP_STATE_TIMEOUT) {
    ESP_LOGI(TAG, "Loop state timeout waiting for response");
    this->current_loop_state = LS_IDLE;
  }

  // If we're not idle, we're awaiting a response, so try to get one of those (queue it for forwarding if it's
  // from the thermostat)
  else {
    if (readPacket(hp_uart, this->current_loop_state == LS_AWAIT_THERMOSTAT_RESPONSE)) {
      // If a packet was read, go back to IDLE
      this->current_loop_state = LS_IDLE;
    }
  }
}

void MitsubishiUART::update() {
  ESP_LOGV(TAG, "Update called.");

  int packetsRead = 0;

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
    // This will publish the state IFF something has changed. Only called if connected
    // so any updates to connection status will need to be done outside this.
    this->climate_->lazy_publish_state(nullptr);

    if (!passive_mode) {
      // Check size just for some sort of sanity check
      if (hp_queue_.size() < 6) {
        // Request room temp
        hp_queue_.push_back(PACKET_TEMP_REQ);
        // Request settings (needs to be done before status for mode logic to work)
        hp_queue_.push_back(PACKET_SETTINGS_REQ);
        // Request status
        hp_queue_.push_back(PACKET_STATUS_REQ);
        // Request standby info
        hp_queue_.push_back(PACKET_STANDBY_REQ);
      }
    }
  }

  if (!passive_mode) {
    if (packetsRead > 0) {
      updatesSinceLastPacket = 0;
    } else {
      updatesSinceLastPacket++;
    }

    if (updatesSinceLastPacket > 10) {
      ESP_LOGI(TAG, "No packets received in %d updates, connection down.", updatesSinceLastPacket);
      connectState = 0;
    }

    // If we're not connected (or have become unconnected) try to send a connect packet again
    if (connectState < 2) {
      connect();
    }
  }
}

void MitsubishiUART::dump_config() {
  ESP_LOGCONFIG(TAG, "Mitsubishi UART v%s", MUART_VERSION);
  ESP_LOGCONFIG(TAG, "Passive: %s Forwarding: %s", YESNO(passive_mode), YESNO(forwarding_));
  ESP_LOGCONFIG(TAG, "Connection state: %d", connectState);
}

void MitsubishiUART::connect() {
  connectState = 1;  // Connecting...
  if (hp_queue_.size() < 6) {
    hp_queue_.push_back(PACKET_CONNECT_REQ);
  }
}

void logPacket(const char *direction, const Packet &packet) {
  // ESP_LOGD(TAG, "%s [%02x] %s", direction, packet.getPacketType(),
  //          format_hex_pretty(&packet.getBytes()[PACKET_HEADER_SIZE], packet.getLength() -
  //          PACKET_HEADER_SIZE).c_str());
}

// Send packet on designated UART interface (as long as it exists, regardless of connection state)
bool MitsubishiUART::sendPacket(Packet packet, uart::UARTComponent *uart) {
  if (!uart) {
    return false;
  }
  uart->write_array(packet.getBytes(), packet.getLength());
  return true;  // I don't think there's anyway to confirm this was actually sent, if the UART exists, assume it was.
}

/**
 * Attempt to read a packet from the specified UART, process it, and queue for forwarding (if needed).
 * Returns true if a packet was read REGARDLESS OF SUCCESS.
 */
bool MitsubishiUART::readPacket(uart::UARTComponent *uart, bool isExternalPacket) {
  // Sanity check
  if (!uart) {
    return false;
  }

  uint8_t packetBytes[PACKET_MAX_SIZE];
  packetBytes[0] = 0;  // Reset control byte before starting

  // Look for a control byte
  while (uart->available() > PACKET_HEADER_SIZE && uart->read_byte(&packetBytes[0])) {
    if (packetBytes[0] == BYTE_CONTROL) {
      ESP_LOGV(TAG, "Found a packet.");
      break;
    }
  }

  // If we found one...
  if (packetBytes[0] == BYTE_CONTROL) {
    // Wait just a moment
    delay(10);
    // Read the rest of the header
    if (!uart->read_array(&packetBytes[1], PACKET_HEADER_SIZE - 1)) {
      // If it doesn't work, abort read
      return true;
    }

    // Find the payload size
    const int payloadSize = packetBytes[PACKET_HEADER_INDEX_PAYLOAD_SIZE];

    // Read the payload + checksum
    if (!uart->read_array(&packetBytes[PACKET_HEADER_SIZE], payloadSize + 1)) {
      // If it doesn't work, abort read
      return true;
    }

    // Construct a packet
    Packet receivedPacket = Packet(packetBytes, payloadSize + PACKET_HEADER_SIZE + 1);

    // If the checksum is valid...
    if (receivedPacket.isChecksumValid()) {
      // TODO: This is an okay place for logging probably

      // Process it
      processPacket(receivedPacket);

      // If we're forwarding, forward it
      if (this->forwarding_ && isExternalPacket) {
        if (uart == hp_uart) {
          ts_queue_.push_back(receivedPacket);
        } else if (uart == tstat_uart) {
          hp_queue_.push_back(receivedPacket);
        }
      }
    }
    return true;
  }

  // No packet was found
  return false;
}

/**
 * Processess information from packets.
 * TODO: This still currently involves copying all the packet bytes to a new typed-packet which is fairly inefficient.
 * Should try to use a move-constructor instead
 */
void MitsubishiUART::processPacket(Packet &packetToProcess) {
  switch (packetToProcess.getPacketType()) {
    // RESPONSES
    case PacketType::connect_response:
      hResConnect(PacketConnectResponse(packetToProcess.getBytes(), packetToProcess.getLength()));
      break;
    case PacketType::extended_connect_response:
      hResExtendedConnect(PacketExtendedConnectResponse(packetToProcess.getBytes(), packetToProcess.getLength()));
      break;
    case PacketType::get_response:
      switch (packetToProcess.getCommand()) {  // Switch on command type
        case PacketGetCommand::gc_settings:
          hResGetSettings(PacketGetResponseSettings(packetToProcess.getBytes(), packetToProcess.getLength()));
          break;
        case PacketGetCommand::gc_room_temp:
          hResGetRoomTemp(PacketGetResponseRoomTemp(packetToProcess.getBytes(), packetToProcess.getLength()));
          break;
        case PacketGetCommand::gc_four:
          hResGetFour(packetToProcess);
          break;
        case PacketGetCommand::gc_status:
          hResGetStatus(PacketGetResponseStatus(packetToProcess.getBytes(), packetToProcess.getLength()));
          break;
        case PacketGetCommand::gc_standby:
          hResGetStandby(PacketGetResponseStandby(packetToProcess.getBytes(), packetToProcess.getLength()));
          break;
        default:
          ESP_LOGI(TAG, "Unknown get response command %x received.", packetToProcess.getCommand());
      }
      break;

    // REQUESTS (these are solely from the thermostat)
    case PacketType::connect_request:
    case PacketType::extended_connect_request:
    case PacketType::get_request:
      // Nothing to do for these requests
      break;
    case PacketType::set_request:
      switch (packetToProcess.getCommand()) {  // Switch on command type
        case PacketSetCommand::sc_settings:
          // Nothing to do for this
          break;
        case PacketSetCommand::sc_remote_temperature:
          hReqSetRemoteTemperature(
              PacketSetRemoteTemperatureRequest(packetToProcess.getBytes(), packetToProcess.getLength()));
          break;
        default:
          ESP_LOGI(TAG, "Unknown set request command %x received.", packetToProcess.getCommand());
      }
      break;
    default:
      ESP_LOGI(TAG, "Unknown packet type %x received.", packetToProcess.getPacketType());
  }
}

////
// Response Handlers
////

PacketConnectResponse MitsubishiUART::hResConnect(const PacketConnectResponse &packet) {
  // Not sure there's any info in the response.
  connectState = 2;
  ESP_LOGI(TAG, "Connected to heatpump.");

  return packet;
}

PacketExtendedConnectResponse MitsubishiUART::hResExtendedConnect(const PacketExtendedConnectResponse &packet) {
  // TODO : Don't know what's in these
  connectState = 2;
  ESP_LOGI(TAG, "Connected to heatpump.");

  return packet;
}

PacketGetResponseSettings MitsubishiUART::hResGetSettings(const PacketGetResponseSettings &packet) {
  const bool power = packet.getPower();
  if (power) {
    switch (packet.getMode()) {
      case 0x01:
        this->climate_->mode = climate::CLIMATE_MODE_HEAT;
        break;
      case 0x02:
        this->climate_->mode = climate::CLIMATE_MODE_DRY;
        break;
      case 0x03:
        this->climate_->mode = climate::CLIMATE_MODE_COOL;
        break;
      case 0x07:
        this->climate_->mode = climate::CLIMATE_MODE_FAN_ONLY;
        break;
      case 0x08:
        this->climate_->mode = climate::CLIMATE_MODE_HEAT_COOL;
        break;
      default:
        this->climate_->mode = climate::CLIMATE_MODE_OFF;
    }
  } else {
    this->climate_->mode = climate::CLIMATE_MODE_OFF;
  }

  this->climate_->target_temperature = packet.getTargetTemp();

  switch (packet.getFan()) {
    case 0x00:
      this->climate_->fan_mode = climate::CLIMATE_FAN_AUTO;
      break;
    case 0x01:
      this->climate_->fan_mode = climate::CLIMATE_FAN_QUIET;
      break;
    case 0x02:
      this->climate_->fan_mode = climate::CLIMATE_FAN_LOW;
      break;
    case 0x03:
      this->climate_->fan_mode = climate::CLIMATE_FAN_MIDDLE;
      break;
    case 0x05:
      this->climate_->fan_mode = climate::CLIMATE_FAN_MEDIUM;
      break;
    case 0x06:
      this->climate_->fan_mode = climate::CLIMATE_FAN_HIGH;
      break;
  }

  uint8_t vane = packet.getVane();
  if (vane > 0x05) {
    vane = 0x06;
  }  // "Swing" is 0x07 and there's no 0x06, so the select menu index only goes to 6
  this->select_vane_direction->lazy_publish_state(this->select_vane_direction->traits.get_options().at(vane));

  const uint8_t h_vane = packet.getHorizontalVane();
  ESP_LOGD(TAG, "HVane set to: %x", h_vane);

  return packet;
}

PacketGetResponseRoomTemp MitsubishiUART::hResGetRoomTemp(const PacketGetResponseRoomTemp &packet) {
  this->climate_->current_temperature = packet.getRoomTemp();
  // I'm starting to suspect that this will always be the same as the remote temperature
  this->sensor_internal_temperature->lazy_publish_state(packet.getRoomTemp());
  ESP_LOGV(TAG, "Room temp: %.1f", this->climate_->current_temperature);

  return packet;
}

Packet MitsubishiUART::hResGetFour(const Packet &packet) {
  // This one is mysterious, keep an eye on it (might just be a ping?)
  int bytesSum = 0;
  for (int i = 6; i < packet.getLength() - 7; i++) {
    bytesSum +=
        packet.getBytes()[i];  // Sum of interesting payload bytes (i.e. not the first one, it's 4, and not checksum)
  }

  ESP_LOGD(TAG, "Get Four returned sum %d", bytesSum);

  return packet;
}

PacketGetResponseStatus MitsubishiUART::hResGetStatus(const PacketGetResponseStatus &packet) {
  const bool operating = packet.getOperating();
  this->sensor_compressor_frequency->lazy_publish_state(packet.getCompressorFrequency());

  // TODO Simplify this switch (too many redundant ACTION_IDLES)
  switch (this->climate_->mode) {
    case climate::CLIMATE_MODE_HEAT:
      if (operating) {
        this->climate_->action = climate::CLIMATE_ACTION_HEATING;
      } else {
        this->climate_->action = climate::CLIMATE_ACTION_IDLE;
      }
      break;
    case climate::CLIMATE_MODE_COOL:
      if (operating) {
        this->climate_->action = climate::CLIMATE_ACTION_COOLING;
      } else {
        this->climate_->action = climate::CLIMATE_ACTION_IDLE;
      }
      break;
    case climate::CLIMATE_MODE_HEAT_COOL:
      this->climate_->action = climate::CLIMATE_ACTION_IDLE;
      if (operating) {
        if (this->climate_->current_temperature > this->climate_->target_temperature) {
          this->climate_->action = climate::CLIMATE_ACTION_COOLING;
        } else if (this->climate_->current_temperature < this->climate_->target_temperature) {
          this->climate_->action = climate::CLIMATE_ACTION_HEATING;
        }
      }
      break;
    case climate::CLIMATE_MODE_DRY:
      if (operating) {
        this->climate_->action = climate::CLIMATE_ACTION_DRYING;
      } else {
        this->climate_->action = climate::CLIMATE_ACTION_IDLE;
      }
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      this->climate_->action = climate::CLIMATE_ACTION_FAN;
      break;
    default:
      this->climate_->action = climate::CLIMATE_ACTION_OFF;
  }

  ESP_LOGD(TAG, "Operating: %s", YESNO(operating));

  return packet;
}

PacketGetResponseStandby MitsubishiUART::hResGetStandby(const PacketGetResponseStandby &packet) {
  // TODO these are a little uncertain
  // 0x04 = pre-heat, 0x08 = standby
  this->sensor_loop_status->lazy_publish_state(packet.getLoopStatus());
  // 1 to 5, lowest to highest power
  this->sensor_stage->lazy_publish_state(packet.getStage());

  return packet;
}

////
//  Handle Requests (received from thermostat)
////
PacketSetRemoteTemperatureRequest MitsubishiUART::hReqSetRemoteTemperature(
    const PacketSetRemoteTemperatureRequest &packet) {
  this->sensor_thermostat_temperature->lazy_publish_state(packet.getRemoteTemperature());
  return packet;
}

}  // namespace mitsubishi_uart
}  // namespace esphome
