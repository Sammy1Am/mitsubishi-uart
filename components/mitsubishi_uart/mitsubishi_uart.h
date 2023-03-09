#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"
#include "muart_packet.h"

namespace esphome {
namespace mitsubishi_uart {

static const char *TAG = "mitsubishi_uart";
static const char *MUART_VERSION = "0.1.0";

const int PACKET_RECEIVE_TIMEOUT = 500; // Milliseconds to wait for a response

const uint8_t MUART_MIN_TEMP = 16; // Degrees C
const uint8_t MUART_MAX_TEMP = 31; // Degrees C
const float MUART_TEMPERATURE_STEP = 0.5;


struct muartState {
  climate::ClimateMode c_mode;
  float c_target_temperature;
  climate::ClimateFanMode c_fan_mode;
  float c_current_temperature;
  climate::ClimateAction c_action;
  
  // power
  // vane
  // hvane
  // operating
  // compressor_frequency
};

class MitsubishiUART : public climate::Climate, public PollingComponent {
 public:
  /**
   * Create a new MitsubishiUART with the specified esphome::uart::UARTComponent.
   */
  MitsubishiUART(uart::UARTComponent *uart_comp);

  // Set up the component, initializing the HeatPump object.
  void setup() override;

  // ?
  climate::ClimateTraits traits() override;
  climate::ClimateTraits& config_traits();

  // ?
  void control(const climate::ClimateCall &call) override;

  // Called periodically as PollingComponent
  void update() override;

  void dump_config() override;

 private:
  uart::UARTComponent *hp_uart;
  uint8_t updatesSinceLastPacket = 0;
  muartState lastPublishedState {};
  muartState getCurrentState();

  climate::ClimateTraits _traits;

  uint8_t connectState = 0;

  void connect();

  // Sends a packet and by default attempts to receive one.  Returns true if a packet was received
  // Caveat: No attempt is made to match received packet with sent packet
  bool sendPacket(Packet packet, bool expectResponse=true); 
  bool readPacket(bool waitForPacket=true);  //TODO separate methods or arguments for HP vs tstat?

  // Packet response handling
  void hResConnect(PacketConnectResponse packet);
  void hResGetSettings(PacketGetResponseSettings packet);
  void hResGetRoomTemp(PacketGetResponseRoomTemp packet);
  void hResGetStatus(PacketGetResponseStatus packet);
  void hResGetStandby(PacketGetResponseStandby packet);
};

}  // namespace mitsubishi_uart
}  // namespace esphome