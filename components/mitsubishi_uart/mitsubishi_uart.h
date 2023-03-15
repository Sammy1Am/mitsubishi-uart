#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/select/select.h"
#include "esphome/components/sensor/sensor.h"
#include "muart_packet.h"

namespace esphome {
namespace mitsubishi_uart {

static const char *TAG = "mitsubishi_uart";
static const char *MUART_VERSION = "0.2.0";

const int PACKET_RECEIVE_TIMEOUT = 500;  // Milliseconds to wait for a response

class MitsubishiUART;

/**
 * A wrapper for components to provide a set_parent(MitsubishiUART) method, and lazy_publish_state(state).
 * The latter is provided because the heat pump does not support "pushing" changes to the microcontroller,
 * so we're polling every few seconds; but don't need to provide published updates quite that often.
 */
template<typename BASECOMPONENT, typename STATETYPE> class MUARTComponent : public BASECOMPONENT {
  // static_assert(std::is_base_of<Component, BASECOMPONENT>::value, "BASECOMPONENT must derive from Component");
 public:
  // Sets parent MitsubishiUART where control commands will be sent (only really needed for controls, not sensors)
  void set_parent(MitsubishiUART *parent) { this->parent_ = parent; }
  // Determines if new provided state differs from current state, and then publishes IFF it is.
  virtual void lazy_publish_state(STATETYPE new_state) = 0;

 protected:
  MitsubishiUART *parent_;
};

class MitsubishiUART : public PollingComponent {
 public:
  /**
   * Create a new MitsubishiUART with the specified esphome::uart::UARTComponent.
   */
  MitsubishiUART(uart::UARTComponent *uart_comp);

  // Called repeatedly (used for UART receiving/forwarding)
  void loop() override;

  // Called periodically as PollingComponent (used for UART sending periodically)
  void update() override;

  void dump_config() override;

  void set_climate(MUARTComponent<climate::Climate, void *> *c) { this->climate_ = c; }
  void set_select_vane_direction(MUARTComponent<select::Select, const std::string &> *svd) {
    this->select_vane_direction = svd;
  }

  void set_sensor_internal_temperature(MUARTComponent<sensor::Sensor, float> *s) {
    this->sensor_internal_temperature = s;
  }
  void set_sensor_loop_status(MUARTComponent<sensor::Sensor, float> *s) { this->sensor_loop_status = s; }
  void set_sensor_stage(MUARTComponent<sensor::Sensor, float> *s) { this->sensor_stage = s; }
  void set_sensor_compressor_frequency(MUARTComponent<sensor::Sensor, float> *s) {
    this->sensor_compressor_frequency = s;
  }

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

  MUARTComponent<climate::Climate, void *> *climate_{};
  MUARTComponent<select::Select, const std::string &> *select_vane_direction{};

  MUARTComponent<sensor::Sensor, float> *sensor_internal_temperature{};
  MUARTComponent<sensor::Sensor, float> *sensor_loop_status{};
  MUARTComponent<sensor::Sensor, float> *sensor_stage{};
  MUARTComponent<sensor::Sensor, float> *sensor_compressor_frequency{};
};

}  // namespace mitsubishi_uart
}  // namespace esphome
