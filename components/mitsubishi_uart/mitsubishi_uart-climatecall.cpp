#include "mitsubishi_uart.h"

namespace esphome {
namespace mitsubishi_uart {

// Called to instruct a change of the climate controls
void MitsubishiUART::control(const climate::ClimateCall &call) {
  SettingsSetRequestPacket setRequestPacket = SettingsSetRequestPacket();

  if (call.get_custom_fan_mode().has_value()) {
    if (call.get_custom_fan_mode().value() == FAN_MODE_VERYHIGH) {
      setRequestPacket.setFan(SettingsSetRequestPacket::FAN_4);
    }
  }

  switch(call.get_fan_mode().value()) {
    case climate::CLIMATE_FAN_QUIET:
      setRequestPacket.setFan(SettingsSetRequestPacket::FAN_QUIET);
      break;
    case climate::CLIMATE_FAN_LOW:
      setRequestPacket.setFan(SettingsSetRequestPacket::FAN_1);
      break;
    case climate::CLIMATE_FAN_MEDIUM:
      setRequestPacket.setFan(SettingsSetRequestPacket::FAN_2);
      break;
    case climate::CLIMATE_FAN_HIGH:
      setRequestPacket.setFan(SettingsSetRequestPacket::FAN_3);
      break;
    case climate::CLIMATE_FAN_AUTO:
      setRequestPacket.setFan(SettingsSetRequestPacket::FAN_AUTO);
      break;
  }

  switch(call.get_mode().value()) {
    // TODO:
  }

  if (call.get_target_temperature().has_value()) {
    setRequestPacket.setTargetTemperature(call.get_target_temperature().value());
  }

  // TODO:
  // Vane
  // HVane?
  // Swing?

  // We're assuming that every climate call *does* make some change worth sending to the heat pump
  hp_bridge.sendPacket(setRequestPacket);
};

}  // namespace mitsubishi_uart
}  // namespace esphome
