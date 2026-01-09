#include "bestway_spa.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bestway_spa {

static const char *const TAG = "bestway_spa";

void BestwaySpa::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Bestway Spa Climate...");

  // Initialize CIO Protocol (Mandatory)
  if (this->protocol_type_ == PROTOCOL_6WIRE_T1) {
    this->va_cio_type1.setup(
      this->cio_data_pin_->get_pin(),
      this->cio_clk_pin_->get_pin(),
      this->cio_cs_pin_->get_pin()
    );
  }

  // Initialize DSP Protocol (Optional, based on YAML config)
  if (this->dsp_data_pin_ != nullptr && this->dsp_clk_pin_ != nullptr && this->dsp_cs_pin_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  DSP Pins detected, initializing Display listener...");
    this->va_dsp_type1.setup(
      this->dsp_data_pin_->get_pin(),
      this->dsp_clk_pin_->get_pin(),
      this->dsp_cs_pin_->get_pin(),
      255 // Audio pin disabled by default
    );
  }
}

void BestwaySpa::loop() {
  // Update the protocol handlers
  if (this->protocol_type_ == PROTOCOL_6WIRE_T1) {
    this->va_cio_type1.updateStates();
    
    // If DSP is active, we could sync display data here in the future
    // For now, we update the climate sensors from the CIO state
    this->update_sensors();
  }
}

void BestwaySpa::update_sensors() {
  // Pull temperatures from the CIO protocol handler
  float target = this->va_cio_type1.get_target_temp();
  float current = this->va_cio_type1.get_current_temp();

  // Only update if we have valid data (avoiding 0.0 on boot)
  if (target > 0) this->target_temperature = target;
  if (current > 0) this->current_temperature = current;
  
  // Sync Heating State
  if (this->va_cio_type1.is_heating()) {
    this->mode = climate::CLIMATE_MODE_HEAT;
    this->action = climate::CLIMATE_ACTION_HEATING;
  } else {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->action = climate::CLIMATE_ACTION_IDLE;
  }
  
  this->publish_state();
}

climate::ClimateTraits BestwaySpa::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_visual_min_temperature(20);
  traits.set_visual_max_temperature(40);
  traits.set_visual_temperature_step(1.0f);
  traits.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_HEAT});
  return traits;
}

void BestwaySpa::control(const climate::ClimateCall &call) {
  if (call.get_target_temperature().has_value()) {
    float target = *call.get_target_temperature();
    // If target is higher than current internal target, simulate UP press
    if (target > this->target_temperature) {
      this->va_cio_type1.setButtonCode(static_cast<uint8_t>(UP));
    } else if (target < this->target_temperature) {
      this->va_cio_type1.setButtonCode(static_cast<uint8_t>(DOWN));
    }
  }

  if (call.get_mode().has_value()) {
    climate::ClimateMode new_mode = *call.get_mode();
    bool heating_currently = this->va_cio_type1.is_heating();

    // Toggle heat if the requested mode doesn't match current state
    if ((new_mode == climate::CLIMATE_MODE_HEAT && !heating_currently) ||
        (new_mode == climate::CLIMATE_MODE_OFF && heating_currently)) {
      this->va_cio_type1.setButtonCode(static_cast<uint8_t>(HEAT));
    }
  }
}

void BestwaySpa::dump_config() {
  ESP_LOGCONFIG(TAG, "Bestway Spa Climate:");
  ESP_LOGCONFIG(TAG, "  Protocol Type: %d", (int)this->protocol_type_);
  LOG_PIN("  CIO Data Pin: ", this->cio_data_pin_);
  LOG_PIN("  CIO Clk Pin: ", this->cio_clk_pin_);
  LOG_PIN("  CIO CS Pin: ", this->cio_cs_pin_);
  
  if (this->dsp_data_pin_ != nullptr) {
    LOG_PIN("  DSP Data Pin: ", this->dsp_data_pin_);
    LOG_PIN("  DSP Clk Pin: ", this->dsp_clk_pin_);
    LOG_PIN("  DSP CS Pin: ", this->dsp_cs_pin_);
  }
}

} // namespace bestway_spa
} // namespace esphome
