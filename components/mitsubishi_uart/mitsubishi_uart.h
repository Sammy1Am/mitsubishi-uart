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

 private:
  uart::UARTComponent *hp_uart;
  uart::UARTComponent *tstat_uart{nullptr};
  uint8_t updatesSinceLastPacket = 0;

  uint8_t connectState = 0;

  // If true, MUART will not generate any packets of its own, only listen and forward them between
  // the heat pump and thermostat.  NOTE: This *only* works if a thermostat is being used, since the
  // heat pump will not send out packets on its own.
  bool passive_mode = false;

  // Should MUART forward thermostat packets (and heat pump responses)
  bool forwarding_ = true;

  void connect();

  /**
   * Sends a packet to the specified UART.  If processResponse is true, will block to read
   * a response packet *and process it* before returning.
   *
   * CAVEAT: This is working on the assumption that processing in this library is NOT
   * concurrent, and all requests will receive a response.
   * (In practice, it seems like there's a read timeout on the serial port so that requests without
   * a response don't break things)
   *
   * If forwardResponse is true, will attempt to forward any response to the other UART interface (e.g.
   * if we're sending to the heat pump, the response will be forwarded to the thermostat).
   */
  bool sendPacket(Packet packet, uart::UARTComponent *uart, bool processResponse, bool forwardResponse);
  /**
   * Reads a packet from the specified UART.  If waitForPacket, will block for PACKET_RECEIVE_TIMEOUT
   * before giving up and returning.  If fowardPacket, will attempt to forward the packet to whichever
   * UART interface the packet wasn't received on (e.g. if it came from the heat pump, we'll forward it
   * to the thermostat).
   */
  bool readPacket(uart::UARTComponent *uart, bool waitForPacket, bool forwardPacket);
  void postprocessPacket(uart::UARTComponent *sourceUART, Packet packet, bool forwardPacket);

  // Packet response handling
  PacketConnectResponse hResConnect(const PacketConnectResponse packet);
  PacketExtendedConnectResponse hResExtendedConnect(const PacketExtendedConnectResponse packet);
  PacketGetResponseSettings hResGetSettings(const PacketGetResponseSettings packet);
  PacketGetResponseRoomTemp hResGetRoomTemp(const PacketGetResponseRoomTemp packet);
  Packet hResGetFour(const Packet packet);
  PacketGetResponseStatus hResGetStatus(const PacketGetResponseStatus packet);
  PacketGetResponseStandby hResGetStandby(const PacketGetResponseStandby packet);

  // Packet request handling
  PacketConnectRequest hReqConnect(const PacketConnectRequest packet);
  PacketExtendedConnectRequest hReqExtendedConnect(const PacketExtendedConnectRequest packet);
  Packet hReqGet(const Packet packet);  // Currently no need to differentiate requests that I'm aware of
  PacketSetSettingsRequest hReqSetSettings(const PacketSetSettingsRequest packet);
  PacketSetRemoteTemperatureRequest hReqSetRemoteTemperature(const PacketSetRemoteTemperatureRequest packet);
  // void hReqGetSettings(PacketGetRequestSettings packet);
  // void hReqGetRoomTemp(PacketGetRequestRoomTemp packet);
  // void hReqGetStatus(PacketGetRequestStatus packet);
  // void hReqGetStandby(PacketGetRequestStandby packet);

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
