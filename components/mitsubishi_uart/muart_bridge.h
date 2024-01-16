#pragma once

#include "esphome/components/uart/uart.h"
#include "muart_packet.h"

namespace esphome {
namespace mitsubishi_uart {

static const char *TAG = "muart_bridge";

// A UARTComponent wrapper to send and receieve packets
class MUARTBridge  {
  public:
    MUARTBridge(uart::UARTComponent *hp_uart_comp, PacketProcessor *packet_processor);

    // Sends a packet, waits for a response, and processes it using the specified PacketProcessor
    void sendAndReceive(Packet *packetToSend);
    void sendPacket(Packet *packetToSend);
    Packet receivePacket();
  private:
    Packet deserializePacket(uint8_t packetBytes[], uint8_t length);

    uart::UARTComponent *hp_uart;
    PacketProcessor *pkt_processor;
};

}  // namespace mitsubishi_uart
}  // namespace esphome
