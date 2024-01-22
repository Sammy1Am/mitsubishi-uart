#pragma once

#include "esphome/components/select/select.h"
#include "mitsubishi_uart.h"


namespace esphome {
namespace mitsubishi_uart {

class MUARTSelect : public select::Select, public Parented<MitsubishiUART> {
  public:
    MUARTSelect() = default;
    using Parented<MitsubishiUART>::Parented;
  protected:
    void control(const std::string &value) override = 0;
};

class TemperatureSourceSelect : public MUARTSelect {
  protected:
    void control(const std::string &value) {
      if (parent_->select_temperature_source(value)) {
        publish_state(value);
      }
    }
};

}  // namespace mitsubishi_uart
}  // namespace esphome
