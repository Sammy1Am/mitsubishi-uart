#include "muart_bridge.h"

namespace esphome {
namespace mitsubishi_uart {

// TODO: This should probably live somewhere common eventually (maybe in MUARTBridge where it'll inherently know the direction)
static void logPacket(const char *direction, const Packet &packet) {
  ESP_LOGD(BRIDGE_TAG, "%s [%02x] %s", direction, packet.getPacketType(),
           format_hex_pretty(&packet.getBytes()[0], packet.getLength()).c_str());
}

MUARTBridge::MUARTBridge(uart::UARTComponent &uart_component, PacketProcessor &packet_processor) : uart_comp{uart_component}, pkt_processor{packet_processor} {}

void MUARTBridge::loop() {

  // Try to get a packet
  if (optional<Packet> pkt = receivePacket()) {
    ESP_LOGD(BRIDGE_TAG, "Received %x packet", pkt.value().getPacketType());
    // Check the packet's checksum and either process it, or log an error
    if (pkt.value().isChecksumValid()) {
      processPacket(pkt.value());
    } else {
      ESP_LOGW(BRIDGE_TAG, "Invalid packet checksum!");
      logPacket("<-HP", pkt.value());
    }

    // If there was a packet waiting for a response, remove it.
    // TODO: This incoming packet wasn't *nessesarily* a response, but for now
    // it's probably not worth checking to make sure it matches.
    if (packetAwaitingResponse.has_value()) {
      packetAwaitingResponse.reset();
    }
  } else if (!packetAwaitingResponse.has_value() && !pkt_queue.empty()) {
    // If we're not waiting for a response and there's a packet in the queue...

    // If the packet expectsa response, add it to the awaitingResponse variable
    if (pkt_queue.front().isResponseExpected()){
      packetAwaitingResponse = pkt_queue.front();
    }

    ESP_LOGD(BRIDGE_TAG, "Sending %x packet", pkt_queue.front().getPacketType());
    writePacket(pkt_queue.front());
    packet_sent_millis = millis();

    // Remove packet from queue
    pkt_queue.pop();
  } else if (packetAwaitingResponse.has_value() && (millis() - packet_sent_millis > RESPONSE_TIMEOUT_MS)) {
    // We've been waiting too long for a response, give up
    // TODO: We could potentially retry here, but that seems unnecessary
    packetAwaitingResponse.reset();
    ESP_LOGW(BRIDGE_TAG, "Timeout waiting for response to  %x packet.", packetAwaitingResponse.value().getPacketType());
  }
}

void MUARTBridge::sendPacket(const Packet &packetToSend) {
  pkt_queue.push(packetToSend);
}

void MUARTBridge::writePacket(const Packet &packetToSend) const {
  uart_comp.write_array(packetToSend.getBytes(), packetToSend.getLength());
}

/* Reads and deserializes a packet from UART.
Communication with heatpump is *slow*, so we need to check and make sure there are
enough packets available before we start reading.  If there aren't enough packets,
no packet will be returned.

Even at 2400 baud, the 100ms readtimeout should be enough to read a whole payload
after the first byte has been received though, so currently we're assuming that once
the header is available, it's safe to call read_array without timing out and severing
the packet.
*/
const optional<Packet> MUARTBridge::receivePacket() const {
  uint8_t packetBytes[PACKET_MAX_SIZE];
  packetBytes[0] = 0;  // Reset control byte before starting

  // Drain UART until we see a control byte (times out after 100ms in UARTComponent)
  while (uart_comp.available() >= PACKET_HEADER_SIZE && uart_comp.read_byte(&packetBytes[0])) {
    if (packetBytes[0] == BYTE_CONTROL) break;
    // TODO: If the serial is all garbage, this may never stop-- we should have our own timeout
  }

  // If we never found a control byte, we didn't receive a packet
  if (packetBytes[0] != BYTE_CONTROL) {
    return nullopt;
  }

  // Read the header
  uart_comp.read_array(&packetBytes[1], PACKET_HEADER_SIZE - 1);

  // Read payload + checksum
  uint8_t payloadSize = packetBytes[PACKET_HEADER_INDEX_PAYLOAD_LENGTH];
  uart_comp.read_array(&packetBytes[PACKET_HEADER_SIZE], payloadSize + 1);

  return Packet(packetBytes, PACKET_HEADER_SIZE + payloadSize + 1);
}

// TODO: Any way to dynamic_cast?
void MUARTBridge::processPacket(Packet &packet) const {
  switch (packet.getPacketType())
  {
  case PacketType::connect_response :
    pkt_processor.processConnectResponsePacket(static_cast<ConnectResponsePacket&>(packet));
    break;
  case PacketType::extended_connect_response :
    pkt_processor.processExtendedConnectResponsePacket(static_cast<ExtendedConnectResponsePacket&>(packet));
    break;
  case PacketType::get_response :
    switch(packet.getCommand()) {
      case GetCommand::gc_current_temp :
       pkt_processor.processCurrentTempGetResponsePacket(static_cast<CurrentTempGetResponsePacket&>(packet));
        break;
      case GetCommand::gc_settings :
       pkt_processor.processSettingsGetResponsePacket(static_cast<SettingsGetResponsePacket&>(packet));
        break;
      case GetCommand::gc_standby :
       pkt_processor.processStandbyGetResponsePacket(static_cast<StandbyGetResponsePacket&>(packet));
        break;
      case GetCommand::gc_status :
        pkt_processor.processStatusGetResponsePacket(static_cast<StatusGetResponsePacket&>(packet));
        break;
      default:
        pkt_processor.processGenericPacket(packet);
    }
    break;
  case PacketType::set_response :
    switch(packet.getCommand()) {
      case SetCommand::sc_remote_temperature :
        pkt_processor.processRemoteTemperatureSetResponsePacket(static_cast<RemoteTemperatureSetResponsePacket&>(packet));
        break;
      default:
        pkt_processor.processGenericPacket(packet);
    }
    break;

  default:
    pkt_processor.processGenericPacket(packet);
  }
}

}  // namespace mitsubishi_uart
}  // namespace esphome
