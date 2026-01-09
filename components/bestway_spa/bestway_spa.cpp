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

  if (this->dsp_data_pin_ != nullptr) {
    this->va_dsp_type1.setup(
      this->dsp_data_pin_->get_pin(),
      this->dsp_clk_pin_->get_pin(),
      this->dsp_cs_pin_->get_pin(),
      255
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
    this->va_cio_type1.setButtonCode(static_cast<uint8_t>(code));
  }
}

void BestwaySpa::update_sensors() {
  float target = this->va_cio_type1.get_target_temp();
  float current = this->va_cio_type1.get_current_temp();

  if (target > 0) {
    this->target_temperature = target;
    if (this->target_temp_sensor_ != nullptr) {
      this->target_temp_sensor_->publish_state(target);
    }
  }
  
  if (current > 0) {
    this->current_temperature = current;
    if (this->current_temp_sensor_ != nullptr) {
      this->current_temp_sensor_->publish_state(current);
    }
  }

  this->mode = this->va_cio_type1.is_heating() ? climate::CLIMATE_MODE_HEAT : climate::CLIMATE_MODE_OFF;
  this->action = this->va_cio_type1.is_heating() ? climate::CLIMATE_ACTION_HEATING : climate::CLIMATE_ACTION_IDLE;

  // Publish binary sensor states
  if (this->heating_sensor_ != nullptr) {
    this->heating_sensor_->publish_state(this->va_cio_type1.is_heating());
  }
  if (this->filter_sensor_ != nullptr) {
    this->filter_sensor_->publish_state(this->va_cio_type1.is_filtering());
  }
  if (this->bubbles_sensor_ != nullptr) {
    this->bubbles_sensor_->publish_state(this->va_cio_type1.is_bubbles());
  }

  // Publish display text
  if (this->display_text_sensor_ != nullptr) {
    this->display_text_sensor_->publish_state(this->va_cio_type1.get_display());
  }

  this->publish_state();
}

climate::ClimateTraits BestwaySpa::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_supported_mode(climate::CLIMATE_MODE_OFF);
  traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);
  traits.set_visual_min_temperature(20);
  traits.set_visual_max_temperature(40);
  traits.set_visual_temperature_step(1.0f);
  return traits;
}

void BestwaySpa::control(const climate::ClimateCall &call) {
  if (call.get_target_temperature().has_value()) {
    float target = *call.get_target_temperature();
    if (target > this->target_temperature) {
      this->on_button_press_(UP);
    } else if (target < this->target_temperature) {
      this->on_button_press_(DOWN);
    }
  }

  if (call.get_mode().has_value()) {
    climate::ClimateMode new_mode = *call.get_mode();
    bool heating_currently = this->va_cio_type1.is_heating();

    if ((new_mode == climate::CLIMATE_MODE_HEAT && !heating_currently) ||
        (new_mode == climate::CLIMATE_MODE_OFF && heating_currently)) {
      this->on_button_press_(HEAT);
    }
  }
}

void BestwaySpa::dump_config() {
  ESP_LOGCONFIG(TAG, "Bestway Spa Climate:");
  ESP_LOGCONFIG(TAG, "  Protocol Type: %d", (int)this->protocol_type_);
}

} // namespace bestway_spa
} // namespace esphome
