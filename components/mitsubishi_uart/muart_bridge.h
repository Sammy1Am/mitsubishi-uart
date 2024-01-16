#pragma once

#include "esphome/components/uart/uart.h"
#include "muart_packet.h"

namespace esphome {
namespace mitsubishi_uart {

static const char *BRIDGE_TAG = "muart_bridge";

// A UARTComponent wrapper to send and receieve packets
class MUARTBridge  {
  public:
    MUARTBridge(uart::UARTComponent &uart_component, PacketProcessor &packet_processor);

    // Sends a packet, waits for a response, and processes it using the specified PacketProcessor
    const void sendAndReceive(const Packet &packetToSend);

    // Sends a packet and returns
    const void sendPacket(const Packet &packetToSend);

    // Reads and processes any waiting packets
    const void receivePackets();

  private:
    const optional<Packet> receivePacket();
    const optional<Packet> deserializePacket(uint8_t packetBytes[], uint8_t length);
    const void processPacket(Packet &packet);

    uart::UARTComponent &uart_comp;
    PacketProcessor &pkt_processor;
};

}  // namespace mitsubishi_uart
}  // namespace esphome
