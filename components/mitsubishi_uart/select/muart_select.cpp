#include "muart_select.h"

namespace esphome {
namespace mitsubishi_uart {

void MUARTSelect::control(const std::string &value) {
  ESP_LOGD(TAG, "Select control called for %s, active is %s", value.c_str(),
           this->traits.get_options().at(this->active_index().value()).c_str());
  parent_->call_select(*this, value);
}

void MUARTSelect::lazy_publish_state(const std::string &value) {
  if (value != lastPublishedState_) {
    this->publish_state(value);
  }
}

// Calls Select::publish_state, and also keeps track of previously published state and preferences
void MUARTSelect::publish_state(const std::string &value) {
  this->Select::publish_state(value);
  lastPublishedState_ = value;
  size_t index = this->active_index().value();
  this->prefs_.save(&index);
}

// Random 32bit value; If this changes existing restore preferences are invalidated
static const uint32_t RESTORE_STATE_VERSION = 0x847EA6ADUL;

void MUARTSelect::restore_state() {
  this->prefs_ = global_preferences->make_preference<size_t>(this->get_object_id_hash() ^ RESTORE_STATE_VERSION);

  size_t recovered_index{};
  if (this->prefs_.load(&recovered_index) && this->has_index(recovered_index)) {
    // Tell MUART that we've changed the selection, if this change is successful, we'll get a lazy_publish_state call
    // back to set the state
    parent_->call_select(*this, this->at(recovered_index).value());
  }

}

}  // namespace mitsubishi_uart
}  // namespace esphome
