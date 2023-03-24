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

const int LOOP_STATE_TIMEOUT = 500;  // Maximum amount of time to wait in one loop state for an action to complete.

enum LOOP_STATE { LS_IDLE, LS_AWAIT_MC_RESPONSE, LS_AWAIT_THERMOSTAT_RESPONSE };
enum CONNECT_STATE { CS_DISCONNECTED, CS_CONNECTING, CS_CONNECTED };

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
  MitsubishiUART(uart::UARTComponent *hp_uart_comp);

  // Called repeatedly (used for UART receiving/forwarding)
  void loop() override;

  // Called periodically as PollingComponent (used for UART sending periodically)
  void update() override;

  void dump_config() override;

  void set_tstat_uart(uart::UARTComponent *tstat_uart_comp) { this->tstat_uart = tstat_uart_comp; }

  void set_passive_mode(bool enable) { this->passive_mode = enable; }
  void set_forwarding(bool enable) { this->forwarding_ = enable; }

  void set_climate(MUARTComponent<climate::Climate, void *> *c) { this->climate_ = c; }
  void set_select_vane_direction(MUARTComponent<select::Select, const std::string &> *svd) {
    this->select_vane_direction = svd;
  }

  void set_sensor_internal_temperature(MUARTComponent<sensor::Sensor, float> *s) {
    this->sensor_internal_temperature = s;
  }
  void set_sensor_thermostat_temperature(MUARTComponent<sensor::Sensor, float> *s) {
    this->sensor_thermostat_temperature = s;
  }
  void set_sensor_loop_status(MUARTComponent<sensor::Sensor, float> *s) { this->sensor_loop_status = s; }
  void set_sensor_stage(MUARTComponent<sensor::Sensor, float> *s) { this->sensor_stage = s; }
  void set_sensor_compressor_frequency(MUARTComponent<sensor::Sensor, float> *s) {
    this->sensor_compressor_frequency = s;
  }

  void call_select(const MUARTComponent<select::Select, const std::string &> &called_select_component,
                   const std::string &new_selection);
  void call_select_vane_direction(const std::string &new_selection);

 private:
  uart::UARTComponent *hp_uart;
  uart::UARTComponent *tstat_uart{nullptr};

  LOOP_STATE current_loop_state = LOOP_STATE::LS_IDLE;
  uint32_t loop_state_start = 0;

  std::deque<Packet> hp_queue_;
  std::deque<Packet> ts_queue_;

  uint8_t updatesSinceLastPacket = 0;
  CONNECT_STATE connect_state = CS_DISCONNECTED;

  // If true, MUART will not generate any packets of its own, only listen and forward them between
  // the heat pump and thermostat.  NOTE: This *only* works if a thermostat is being used, since the
  // heat pump will not send out packets on its own.
  bool passive_mode = false;

  // Should MUART forward thermostat packets (and heat pump responses)
  bool forwarding_ = true;

  void connect();

  /**
   * Sends a packet to the specified UART.  Returns true if successfully sent.
   */
  bool sendPacket(Packet packet, uart::UARTComponent *uart);
  /**
   * Reads a packet from the specified UART.  If waitForPacket, will block for PACKET_RECEIVE_TIMEOUT
   * before giving up and returning.  If fowardPacket, will attempt to forward the packet to whichever
   * UART interface the packet wasn't received on (e.g. if it came from the heat pump, we'll forward it
   * to the thermostat).
   */
  bool readPacket(uart::UARTComponent *uart, bool isExternalPacket = false);
  void processPacket(Packet &packetToProcess);
  void postprocessPacket(uart::UARTComponent *sourceUART, const Packet &packet, bool forwardPacket);

  // Packet response handling
  const PacketConnectResponse &hResConnect(const PacketConnectResponse &packet);
  const PacketExtendedConnectResponse &hResExtendedConnect(const PacketExtendedConnectResponse &packet);
  const PacketGetResponseSettings &hResGetSettings(const PacketGetResponseSettings &packet);
  const PacketGetResponseRoomTemp &hResGetRoomTemp(const PacketGetResponseRoomTemp &packet);
  const Packet &hResGetFour(const Packet &packet);
  const PacketGetResponseStatus &hResGetStatus(const PacketGetResponseStatus &packet);
  const PacketGetResponseStandby &hResGetStandby(const PacketGetResponseStandby &packet);

  // Packet request handling (most requests just get forwarded and don't need any processing)
  // PacketSetSettingsRequest hReqSetSettings(const PacketSetSettingsRequest &packet);
  const PacketSetRemoteTemperatureRequest &hReqSetRemoteTemperature(const PacketSetRemoteTemperatureRequest &packet);

  MUARTComponent<climate::Climate, void *> *climate_{};
  MUARTComponent<select::Select, const std::string &> *select_vane_direction{};

  MUARTComponent<sensor::Sensor, float> *sensor_internal_temperature{};
  MUARTComponent<sensor::Sensor, float> *sensor_thermostat_temperature{};
  MUARTComponent<sensor::Sensor, float> *sensor_loop_status{};
  MUARTComponent<sensor::Sensor, float> *sensor_stage{};
  MUARTComponent<sensor::Sensor, float> *sensor_compressor_frequency{};
};

}  // namespace mitsubishi_uart
}  // namespace esphome
