#include "muart_bridge.h"

namespace esphome {
namespace mitsubishi_uart {

MUARTBridge::MUARTBridge(uart::UARTComponent *uart_component, PacketProcessor *packet_processor) : uart_comp{*uart_component}, pkt_processor{*packet_processor} {}

// The heatpump loop expects responses for most sent packets, so it tracks the last send packet and wait for a response
void HeatpumpBridge::loop() {

  // Try to get a packet
  if (optional<RawPacket> pkt = receiveRawPacket()) {
    ESP_LOGV(BRIDGE_TAG, "Parsing %x heatpump packet", pkt.value().getPacketType());
    // Check the packet's checksum and either process it, or log an error
    if (pkt.value().isChecksumValid()) {
      // If we're waiting for a response, associate the incomming packet with the request packet
      classifyAndProcessRawPacket(pkt.value(), BridgeAssoc::ba_heatpump, packetAwaitingResponse.has_value() ? packetAwaitingResponse.value().associatedController : ControllerAssoc::ca_muart);
    } else {
      ESP_LOGW(BRIDGE_TAG, "Invalid packet checksum!\n%s", format_hex_pretty(&pkt.value().getBytes()[0], pkt.value().getLength()).c_str());
    }

    // If there was a packet waiting for a response, remove it.
    // TODO: This incoming packet wasn't *nessesarily* a response, but for now
    // it's probably not worth checking to make sure it matches.
    if (packetAwaitingResponse.has_value()) {
      packetAwaitingResponse.reset();
    }
  } else if (!packetAwaitingResponse.has_value() && !pkt_queue.empty()) {
    // If we're not waiting for a response and there's a packet in the queue...

    // If the packet expects a response, add it to the awaitingResponse variable
    if (pkt_queue.front().isResponseExpected()){
      packetAwaitingResponse = pkt_queue.front();
    }

    ESP_LOGV(BRIDGE_TAG, "Sending to heatpump %s", pkt_queue.front().to_string().c_str());
    writeRawPacket(pkt_queue.front().rawPacket());
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

// The thermostat bridge loop doesn't expect any responses, so packets in queue are just sent without checking if they expect a response
void ThermostatBridge::loop() {
  // Try to get a packet
  if (optional<RawPacket> pkt = receiveRawPacket()) {
    ESP_LOGV(BRIDGE_TAG, "Parsing %x thermostat packet", pkt.value().getPacketType());
    // Check the packet's checksum and either process it, or log an error
    if (pkt.value().isChecksumValid()) {
      classifyAndProcessRawPacket(pkt.value(), BridgeAssoc::ba_thermostat, ControllerAssoc::ca_thermostat);
    } else {
      ESP_LOGW(BRIDGE_TAG, "Invalid packet checksum!\n%s", format_hex_pretty(&pkt.value().getBytes()[0], pkt.value().getLength()).c_str());
    }
  } else if (!pkt_queue.empty()) {
    // If there's a packet in the queue...

    ESP_LOGV(BRIDGE_TAG, "Sending to thermostat %s", pkt_queue.front().to_string().c_str());
    writeRawPacket(pkt_queue.front().rawPacket());
    packet_sent_millis = millis();

    // Remove packet from queue
    pkt_queue.pop();
  }
}

void MUARTBridge::sendPacket(const Packet &packetToSend) {
  pkt_queue.push(packetToSend);
}

void MUARTBridge::writeRawPacket(const RawPacket &packetToSend) const {
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
const optional<RawPacket> MUARTBridge::receiveRawPacket() const {
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

  return RawPacket(packetBytes, PACKET_HEADER_SIZE + payloadSize + 1);
}

template <class P>
void MUARTBridge::processRawPacket(RawPacket &pkt, BridgeAssoc sourceBridge, ControllerAssoc associatedController, bool expectResponse) const {
  P packet = P(std::move(pkt));
  // TODO: ? If these are used EVERY time a packet is constructed, maybe move to constructor?
  packet.sourceBridge = sourceBridge;
  packet.associatedController = associatedController;
  packet.setResponseExpected(expectResponse);
  pkt_processor.processPacket(packet);
}

void MUARTBridge::classifyAndProcessRawPacket(RawPacket &pkt, BridgeAssoc sB, ControllerAssoc aC) const {
  switch (pkt.getPacketType())
  {
  case PacketType::connect_request :
    processRawPacket<ConnectRequestPacket>(pkt, sB, aC, true);
    break;
  case PacketType::connect_response :
    processRawPacket<ConnectResponsePacket>(pkt, sB, aC, false);
    break;

  case PacketType::extended_connect_request :
    processRawPacket<ExtendedConnectRequestPacket>(pkt, sB, aC, true);
    break;
  case PacketType::extended_connect_response :
    processRawPacket<ExtendedConnectResponsePacket>(pkt, sB, aC, false);
    break;

  case PacketType::get_request :
    processRawPacket<GetRequestPacket>(pkt, sB, aC, true);
    break;
  case PacketType::get_response :
    switch(pkt.getCommand()) {
      case GetCommand::gc_current_temp :
        processRawPacket<CurrentTempGetResponsePacket>(pkt, sB, aC, false);
        break;
      case GetCommand::gc_settings :
        processRawPacket<SettingsGetResponsePacket>(pkt, sB, aC, false);
        break;
      case GetCommand::gc_standby :
        processRawPacket<StandbyGetResponsePacket>(pkt, sB, aC, false);
        break;
      case GetCommand::gc_status :
        processRawPacket<StatusGetResponsePacket>(pkt, sB, aC, false);
        break;
      default:
        processRawPacket<Packet>(pkt, sB, aC, false);
    }
    break;

  case PacketType::set_response :
    switch(pkt.getCommand()) {
      case SetCommand::sc_remote_temperature :
        processRawPacket<RemoteTemperatureSetResponsePacket>(pkt, sB, aC, false);
        break;
      default:
        processRawPacket<Packet>(pkt, sB, aC, false);
    }
    break;

  default:
    processRawPacket<Packet>(pkt, sB, aC, true); // If we get an unknown packet from the thermostat, expect a response
  }
}

}  // namespace mitsubishi_uart
}  // namespace esphome
