#include "muart_packet.h"

namespace esphome {
namespace mitsubishi_uart {

// Creates an empty packet
Packet::Packet() {
  // TODO: Is this okay?
}

std::string Packet::to_string() const {
  return format_hex_pretty(&pkt_.getBytes()[0], pkt_.getLength());
}

//static char format_hex_pretty_char(uint8_t v) { return v >= 10 ? 'A' + (v - 10) : '0' + v; }
// std::string Packet::to_string() const {
//   // Based on `format_hex_pretty` from ESPHome
//   if (pkt_.getLength() < PACKET_HEADER_SIZE)
//     return "";
//   std::string ret;
//   ret.resize((3 * pkt_.getLength() - 1) + 3);

//   // Header bracket
//   ret[0] = '[';
//   // Header
//   for (size_t i = 0; i < PACKET_HEADER_SIZE; i++) {
//     ret[(3 * i) + 1] = format_hex_pretty_char((pkt_.getBytes()[i] & 0xF0) >> 4);
//     ret[(3 * i) + 2] = format_hex_pretty_char(pkt_.getBytes()[i] & 0x0F);
//   }
//   // Header close-bracket
//   ret[(3*PACKET_HEADER_SIZE)+1] = ']';

//   // Payload
//   for (size_t i = PACKET_HEADER_SIZE; i < pkt_.getLength()-1; i++) {
//     ret[(3 * i) + 2] = format_hex_pretty_char((pkt_.getBytes()[i] & 0xF0) >> 4);
//     ret[(3 * i) + 3] = format_hex_pretty_char(pkt_.getBytes()[i] & 0x0F);
//   }

//   // Space
//   ret[(3*(pkt_.getLength()-1))+2] = ' ';

//   // Checksum
//   ret[(3 * pkt_.getLength()) + 3] = format_hex_pretty_char(pkt_.getBytes()[pkt_.getLength()-1]);

//   return ret;
// }

// std::string Packet::to_string() const {
//   // Based on `format_hex_pretty` from ESPHome
//   if (pkt_.getLength() < PACKET_HEADER_SIZE)
//     return "";
//   std::stringstream stream;

//   stream << (ESPHOME_LOG_COLOR(ESPHOME_LOG_COLOR_CYAN)+'[');

//   for (size_t i = 0; i < PACKET_HEADER_SIZE; i++) {
//     stream << format_hex_pretty_char((pkt_.getBytes()[i] & 0xF0) >> 4);
//     stream << format_hex_pretty_char(pkt_.getBytes()[i] & 0x0F);
//     if (i<PACKET_HEADER_SIZE-1){
//       stream << '.';
//     }
//   }
//   // Header close-bracket
//   stream << (']' + ESPHOME_LOG_COLOR(ESPHOME_LOG_COLOR_WHITE));

//   // Payload
//   for (size_t i = PACKET_HEADER_SIZE; i < pkt_.getLength()-1; i++) {
//     stream << format_hex_pretty_char((pkt_.getBytes()[i] & 0xF0) >> 4);
//     stream << format_hex_pretty_char(pkt_.getBytes()[i] & 0x0F);
//     if (i<pkt_.getLength()-2){
//       stream << '.';
//     }
//   }

//   // Space
//   stream << ' ' + ESPHOME_LOG_COLOR(ESPHOME_LOG_COLOR_GRAY);

//   // Checksum
//   stream << format_hex_pretty_char((pkt_.getBytes()[pkt_.getLength()-1] & 0xF0) >> 4);
//   stream << format_hex_pretty_char(pkt_.getBytes()[pkt_.getLength()-1] & 0x0F);

//   return stream.str();
// }


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
