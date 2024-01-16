#include "muart_bridge.h"

namespace esphome {
namespace mitsubishi_uart {

// TODO: This should probably live somewhere common eventually (maybe in MUARTBridge where it'll inherently know the direction)
static void logPacket(const char *direction, const Packet &packet) {
  ESP_LOGD(BRIDGE_TAG, "%s [%02x] %s", direction, packet.getPacketType(),
           format_hex_pretty(&packet.getBytes()[0], packet.getLength()).c_str());
}

MUARTBridge::MUARTBridge(uart::UARTComponent &uart_component, PacketProcessor &packet_processor) : uart_comp{uart_component}, pkt_processor{packet_processor} {}

const void MUARTBridge::sendAndReceive(const Packet &packetToSend) {
  sendPacket(packetToSend);
  if (optional<Packet> pkt = receivePacket()) {
    pkt.value().process(pkt_processor);
  } else {
    ESP_LOGW(BRIDGE_TAG, "No response to %x type packet received.", packetToSend.getPacketType());
  }
}

const void MUARTBridge::sendPacket(const Packet &packetToSend) {
  uart_comp.write_array(packetToSend.getBytes(), packetToSend.getLength());
}

const void MUARTBridge::receivePackets() {
  while (optional<Packet> pkt = receivePacket()) {
    pkt.value().process(pkt_processor);
  }
}

const optional<Packet> MUARTBridge::receivePacket() {
  uint8_t packetBytes[PACKET_MAX_SIZE];
  packetBytes[0] = 0;  // Reset control byte before starting

  // Drain UART until we see a control byte (times out after 100ms in UARTComponent)
  while (uart_comp.read_byte(&packetBytes[0])) {
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
  // TODO: This might timeout because 2400 baud is too slow, figure out something to do with that...
  uint8_t payloadSize = packetBytes[PACKET_HEADER_INDEX_PAYLOAD_LENGTH];
  uart_comp.read_array(&packetBytes[PACKET_HEADER_SIZE], payloadSize + 1);

  // TODO: Somewhere we should check the checksum.  Is it here?  Is it in the processor??
  Packet returnPacket = deserializePacket(packetBytes, PACKET_HEADER_SIZE + payloadSize + 1);

  // If the checksum is invalid, don't return this packet
  // TODO: Add an override option here if we're expecting bad checksums
  if (!returnPacket.isChecksumValid()) {
    ESP_LOGW(BRIDGE_TAG, "Invalid packet checksum!");
    logPacket("<-HP", returnPacket);
    return nullopt;
  }

  return returnPacket;
}

const Packet MUARTBridge::deserializePacket(uint8_t packetBytes[], uint8_t length) {
  switch (packetBytes[PACKET_HEADER_INDEX_PACKET_TYPE])
  {
  case PacketType::connect_response :
    return ConnectResponsePacket(packetBytes,length);
    break;
  case PacketType::extended_connect_response :
    return ConnectResponsePacket(packetBytes,length);
    break;
  case PacketType::get_response :
    // The first byte of the payload is the "command"
    switch(packetBytes[PACKET_HEADER_SIZE + 1]) {
      case GetCommand::gc_room_temp :
        return RoomTempGetResponsePacket(packetBytes,length);
        break;
      case GetCommand::gc_settings :
        return SettingsGetResponsePacket(packetBytes,length);
        break;
      case GetCommand::gc_standby :
        return StandbyGetResponsePacket(packetBytes,length);
        break;
      case GetCommand::gc_status :
        return StatusGetResponsePacket(packetBytes,length);
        break;
      default:
        return Packet(packetBytes,length);
    }
    break;
  case PacketType::set_response :
    // The first byte of the payload is the "command"
    switch(packetBytes[PACKET_HEADER_SIZE + 1]) {
      case SetCommand::sc_remote_temperature :
        return RemoteTemperatureSetResponsePacket(packetBytes,length);
        break;
      default:
        return Packet(packetBytes,length);
    }
    break;

  default:
    return Packet(packetBytes,length);
  }
}

}  // namespace mitsubishi_uart
}  // namespace esphome
