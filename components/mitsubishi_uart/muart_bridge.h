#pragma once

#include "esphome/components/uart/uart.h"
#include "muart_packet.h"
#include "queue"

namespace esphome {
namespace mitsubishi_uart {

static const char *BRIDGE_TAG = "muart_bridge";
static const uint32_t RESPONSE_TIMEOUT_MS = 3000; // Maximum amount of time to wait for an expected response packet

// A UARTComponent wrapper to send and receieve packets
class MUARTBridge  {
  public:
    MUARTBridge(uart::UARTComponent &uart_component, PacketProcessor &packet_processor);

    // Enqueues a packet to be sent
    void sendPacket(const Packet &packetToSend);

    // Checks for incoming packets, processes them, sends queued packets
    void loop();

  private:
    const optional<Packet> receivePacket() const;
    void writePacket(const Packet &packet) const;
    void processPacket(Packet &packet) const;

    uart::UARTComponent &uart_comp;
    PacketProcessor &pkt_processor;
    std::queue<Packet> pkt_queue;
    optional<Packet> packetAwaitingResponse = nullopt;
    uint32_t packet_sent_millis;
};

}  // namespace mitsubishi_uart
}  // namespace esphome
