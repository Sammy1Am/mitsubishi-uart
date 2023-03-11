#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/select/select.h"
#include "muart_packet.h"

namespace esphome {
namespace mitsubishi_uart {

static const char *TAG = "mitsubishi_uart";
static const char *MUART_VERSION = "0.2.0";

const int PACKET_RECEIVE_TIMEOUT = 500;  // Milliseconds to wait for a response

/**
 * A light wrapper around the Climate class to only publish state when something has changed.
 * This is especially useful for Mitsubishi heat pumps as there's no way for the heat pump to
 * "push" updates to a client, so to keep things responsive we're polling the unit every few seconds.
 * Publishing updates every few seconds is excessive though, so this helps with that.
*/
class LazyClimate : public climate::Climate {
  // Struct to store (relevant) state of the Climate object to determine if publishing is needed.
  struct climateState {
    climate::ClimateMode c_mode;
    float c_target_temperature;
    climate::ClimateFanMode c_fan_mode;
    float c_current_temperature;
    climate::ClimateAction c_action;
  };

  public:
    void lazy_publish_state(){
      climateState currentState = this->getCurrentState();
      if (stateDiffers(currentState, this->lastPublishedState_)) {
        publish_state();
        lastPublishedState_ = currentState;
      }
    }
  private:
    climateState getCurrentState(){
      climateState currentState{};
      currentState.c_action = this->action;
      currentState.c_current_temperature = this->current_temperature;
      currentState.c_fan_mode = this->fan_mode.value_or(climate::ClimateFanMode::CLIMATE_FAN_OFF);
      currentState.c_mode = this->mode;
      currentState.c_target_temperature = this->target_temperature;
      return currentState;
    }
    climateState lastPublishedState_;
    bool stateDiffers(const climateState &lhs, const climateState &rhs){
      return lhs.c_action != rhs.c_action || lhs.c_current_temperature != rhs.c_current_temperature ||
        lhs.c_fan_mode != rhs.c_fan_mode || lhs.c_mode != rhs.c_mode ||
        lhs.c_target_temperature != rhs.c_target_temperature;
    }
};

/**
 * A light wrapper around the Select class to only publish state when something has changed.
 * This is especially useful for Mitsubishi heat pumps as there's no way for the heat pump to
 * "push" updates to a client, so to keep things responsive we're polling the unit every few seconds.
 * Publishing updates every few seconds is excessive though, so this helps with that.
*/
class LazySelect : public select::Select {
  public:
    // Only publishes if this state would be different than the current one
    void lazy_publish_state(const std::string &state){
      if (state != this->traits.get_options().at(this->active_index().value())){ // TODO How safe is calling .value() like this?
      publish_state(state);
      }
    };
};

class MitsubishiUART : public PollingComponent {
 public:
  /**
   * Create a new MitsubishiUART with the specified esphome::uart::UARTComponent.
   */
  MitsubishiUART(uart::UARTComponent *uart_comp);

  // Called periodically as PollingComponent
  void update() override;

  void dump_config() override;

  void set_climate(climate::Climate *c) { this->climate_ = c;}
  void set_select_vane_direction(LazySelect *svd) { this->select_vane_direction = svd; }

 private:
  uart::UARTComponent *hp_uart;
  uint8_t updatesSinceLastPacket = 0;

  uint8_t connectState = 0;

  void connect();

  // Sends a packet and by default attempts to receive one.  Returns true if a packet was received
  // Caveat: No attempt is made to match received packet with sent packet
  bool sendPacket(Packet packet, bool expectResponse = true);
  bool readPacket(bool waitForPacket = true);  // TODO separate methods or arguments for HP vs tstat?

  // Packet response handling
  void hResConnect(PacketConnectResponse packet);
  void hResGetSettings(PacketGetResponseSettings packet);
  void hResGetRoomTemp(PacketGetResponseRoomTemp packet);
  void hResGetStatus(PacketGetResponseStatus packet);
  void hResGetStandby(PacketGetResponseStandby packet);

  climate::Climate *climate_{};
  LazySelect *select_vane_direction{};
};

}  // namespace mitsubishi_uart
}  // namespace esphome
