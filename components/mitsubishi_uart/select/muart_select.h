#pragma once

#include "../mitsubishi_uart.h"
#include "esphome/components/select/select.h"

namespace esphome{
namespace mitsubishi_uart{

class MUARTSelect : public select::Select, public Component {
  public:
    void set_parent(MitsubishiUART *parent) {this->parent_ = parent;}
    void control(const std::string &value) override;
  protected:
    MitsubishiUART *parent_;
};

}
}