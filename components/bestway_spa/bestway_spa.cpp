#include "bestway_spa.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bestway_spa {

static const char *const TAG = "bestway_spa";

void BestwaySpa::setup() {
  if (this->protocol_type_ == PROTOCOL_6WIRE_T1) {
    this->va_cio_type1.setup(
      this->cio_data_pin_->get_pin(),
      this->cio_clk_pin_->get_pin(),
      this->cio_cs_pin_->get_pin()
    );
  }
}

void BestwaySpa::loop() {
  if (this->protocol_type_ == PROTOCOL_6WIRE_T1) {
    this->va_cio_type1.updateStates();
    this->update_sensors();
  }
}

void BestwaySpa::on_button_press_(ButtonCode code) {
  if (this->protocol_type_ == PROTOCOL_6WIRE_T1) {
    this->va_cio_type1.setButtonCode(code);
  }
}

void BestwaySpa::update_sensors() {
  this->target_temperature = this->va_cio_type1.get_target_temp();
  this->current_temperature = this->va_cio_type1.get_current_temp();
  
  this->mode = this->va_cio_type1.is_heating() ? climate::CLIMATE_MODE_HEAT : climate::CLIMATE_MODE_OFF;
  this->action = this->va_cio_type1.is_heating() ? climate::CLIMATE_ACTION_HEATING : climate::CLIMATE_ACTION_IDLE;
  
  this->publish_state();
}

climate::ClimateTraits BestwaySpa::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_visual_min_temperature(20);
  traits.set_visual_max_temperature(40);
  traits.set_visual_temperature_step(1);
  traits.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_HEAT});
  return traits;
}

void BestwaySpa::control(const climate::ClimateCall &call) {
  if (call.get_target_temperature().has_value()) {
    float target = *call.get_target_temperature();
    // Simplified logic: If target > current, press UP
    if (target > this->target_temperature) {
      this->on_button_press_(UP);
    } else if (target < this->target_temperature) {
      this->on_button_press_(DOWN);
    }
  }
}

void BestwaySpa::dump_config() {
  ESP_LOGCONFIG(TAG, "Bestway Spa:");
  ESP_LOGCONFIG(TAG, "  Protocol: %d", this->protocol_type_);
}

} // namespace bestway_spa
} // namespace esphome
