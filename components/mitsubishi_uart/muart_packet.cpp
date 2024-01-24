#include "muart_packet.h"

namespace esphome {
namespace mitsubishi_uart {

// Creates an empty packet
Packet::Packet() {
  // TODO: Is this okay?
}

void Packet::setFlags(const uint8_t flagValue) {
  pkt_.setPayloadByte(PLINDEX_FLAGS, flagValue);
}

// Adds a flag (ONLY APPLICABLE FOR SOME COMMANDS)
void Packet::addFlag(const uint8_t flagToAdd) {
  pkt_.setPayloadByte(PLINDEX_FLAGS, pkt_.getPayloadByte(PLINDEX_FLAGS) | flagToAdd);
}
// Adds a flag2 (ONLY APPLICABLE FOR SOME COMMANDS)
void Packet::addFlag2(const uint8_t flag2ToAdd) {
  pkt_.setPayloadByte(PLINDEX_FLAGS2, pkt_.getPayloadByte(PLINDEX_FLAGS2) | flag2ToAdd);
}

}  // namespace mitsubishi_uart
}  // namespace esphome
