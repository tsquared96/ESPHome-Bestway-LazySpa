#include "bestway_spa.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bestway_spa {

static const char *const TAG = "bestway_spa";

void BestwaySpa::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Bestway Spa...");

  if (this->protocol_type_ == PROTOCOL_6WIRE_T1) {
    // 1. Initialize the CIO (Pump Side) Driver
    this->va_cio_type1.setup(
      this->cio_data_pin_->get_pin(), 
      this->cio_clk_pin_->get_pin(), 
      this->cio_cs_pin_->get_pin()
    );

    // 2. Initialize the DSP (Display Side) Driver if pins are provided
    if (this->dsp_clk_pin_ != nullptr) {
      int audio = (this->audio_pin_ != nullptr) ? this->audio_pin_->get_pin() : -1;
      this->va_dsp_type1.setup(
        this->dsp_data_pin_->get_pin(), 
        this->dsp_clk_pin_->get_pin(), 
        this->dsp_cs_pin_->get_pin(), 
        audio
      );
      this->dsp_enabled_ = true;
    }

    // 3. Attach Global Interrupts
    // These must point to the static wrapper functions in your .h file
    auto *cio = &this->va_cio_type1;
    this->cio_clk_pin_->setup();
    this->cio_cs_pin_->setup();
    
    // Note: In a real implementation, you'd use a global pointer to 'this' 
    // for the ISRs to access va_cio_type1.
  }
}

void BestwaySpa::loop() {
  if (this->protocol_type_ == PROTOCOL_6WIRE_T1) {
    // Update the internal state structures from the last captured packets
    this->va_cio_type1.updateStates();
    if (this->dsp_enabled_) {
      this->va_dsp_type1.handleStates();
    }

    // Sync state from driver to ESPHome Climate
    this->current_temperature = (float)this->va_cio_type1.cio_states.temperature;
    this->target_temperature = (float)this->va_cio_type1.cio_states.target;

    // Handle incoming button commands from the queue
    if (!this->button_queue_.empty()) {
        bestway_va::Buttons btn = this->button_queue_.front();
        uint16_t code = this->va_cio_type1.getButtonCode(btn);
        
        // Inject the code into the driver for the next packet
        this->va_cio_type1.setButtonCode(code);
        this->button_queue_.pop();
    } else {
        // Reset to NOBTN code if nothing is queued
        this->va_cio_type1.setButtonCode(this->va_cio_type1.getButtonCode(bestway_va::NOBTN));
    }
  }
}

void BestwaySpa::control(const climate::ClimateCall &call) {
  if (call.get_target_temperature().has_value()) {
    float temp = *call.get_target_temperature();
    // Logic to queue UP/DOWN button presses until target is reached
    ESP_LOGD(TAG, "Target temperature change requested: %.1f", temp);
  }
}

void BestwaySpa::on_button_press_(bestway_va::Buttons button) {
  ESP_LOGD(TAG, "Button pressed: %d", button);
  this->button_queue_.push(button);
}

void BestwaySpa::dump_config() {
  LOG_CLIMATE("", "Bestway Spa", this);
  ESP_LOGCONFIG(TAG, "  Protocol: %s", (this->protocol_type_ == PROTOCOL_6WIRE_T1) ? "6-WIRE TYPE 1" : "OTHER");
}

}  // namespace bestway_spa
}  // namespace esphome
