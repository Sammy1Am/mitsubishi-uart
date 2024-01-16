#include "muart_bridge.h"

namespace esphome {
namespace mitsubishi_uart {

MUARTBridge::MUARTBridge(uart::UARTComponent *uart_comp, PacketProcessor *packet_processor) : hp_uart{uart_comp}, pkt_processor{packet_processor} {}

void MUARTBridge::sendAndReceive(Packet *packetToSend) {
  sendPacket(packetToSend);
  Packet pkt = receivePacket();
  pkt.process(pkt_processor);
}

void MUARTBridge::sendPacket(Packet *packetToSend) {
  hp_uart->write_array(packetToSend->getBytes(), packetToSend->getLength());
}

Packet MUARTBridge::receivePacket() {
  uint8_t packetBytes[PACKET_MAX_SIZE];
  packetBytes[0] = 0;  // Reset control byte before starting

  // Drain UART until we see a control byte
  while (hp_uart->read_byte(&packetBytes[0])) {
    if (packetBytes[0] == BYTE_CONTROL) break;
  }

  // Read the header
  hp_uart->read_array(&packetBytes[1], PACKET_HEADER_SIZE - 1);

  // Read payload + checksum
  // TODO: This might timeout because 2400 baud is too slow, figure out something to do with that...
  uint8_t payloadSize = packetBytes[PACKET_HEADER_INDEX_PAYLOAD_LENGTH];
  hp_uart->read_array(&packetBytes[PACKET_HEADER_SIZE], payloadSize + 1);

  // TODO: Somewhere we should check the checksum.  Is it here?  Is it in the processor??
  return deserializePacket(packetBytes, PACKET_HEADER_SIZE + payloadSize + 1);
}

Packet MUARTBridge::deserializePacket(uint8_t packetBytes[], uint8_t length) {
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
      case GetCommand::room_temp :
        return RoomTempGetResponsePacket(packetBytes,length);
        break;
      case GetCommand::settings :
        return SettingsGetResponsePacket(packetBytes,length);
        break;
      case GetCommand::standby :
        return StandbyGetResponsePacket(packetBytes,length);
        break;
      case GetCommand::status :
        return StatusGetResponsePacket(packetBytes,length);
        break;
      default:
        return Packet(packetBytes,length);
    }
    break;
  case PacketType::set_response :
    // The first byte of the payload is the "command"
    switch(packetBytes[PACKET_HEADER_SIZE + 1]) {
      case SetCommand::remote_temperature :
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
