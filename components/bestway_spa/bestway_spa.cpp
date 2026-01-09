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

    // 2. Prepare pins for interrupts
    this->cio_clk_pin_->setup();
    this->cio_cs_pin_->setup();
    this->cio_data_pin_->setup(); // Ensure data pin is initialized as input

    // 3. Attach CIO (Pump) Interrupts
    // These lambdas bridge the global interrupt to your specific class instance
    attachInterrupt(this->cio_clk_pin_->get_pin(), [this]() { this->va_cio_type1.isr_clkHandler(); }, CHANGE);
    attachInterrupt(this->cio_cs_pin_->get_pin(), [this]() { this->va_cio_type1.isr_packetHandler(); }, CHANGE);

    // 4. Initialize the DSP (Display Side) Driver if pins are provided
    if (this->dsp_clk_pin_ != nullptr) {
      int audio = (this->audio_pin_ != nullptr) ? this->audio_pin_->get_pin() : -1;
      this->va_dsp_type1.setup(
        this->dsp_data_pin_->get_pin(), 
        this->dsp_clk_pin_->get_pin(), 
        this->dsp_cs_pin_->get_pin(), 
        audio
      );

      this->dsp_clk_pin_->setup();
      this->dsp_cs_pin_->setup();
      this->dsp_data_pin_->setup();

      // Attach DSP (Display) Interrupts
      attachInterrupt(this->dsp_clk_pin_->get_pin(), [this]() { this->va_dsp_type1.isr_clkHandler(); }, CHANGE);
      attachInterrupt(this->dsp_cs_pin_->get_pin(), [this]() { this->va_dsp_type1.isr_packetHandler(); }, CHANGE);
      
      this->dsp_enabled_ = true;
    }
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
    // Note: If temperature shows as ASCII codes, we may need to convert here
    this->current_temperature = (float)this->va_cio_type1.cio_states.temperature;
    this->target_temperature = (float)this->va_cio_type1.cio_states.target;

    // Handle incoming button commands from the queue
    if (!this->button_queue_.empty()) {
        bestway_va::Buttons btn = this->button_queue_.front();
        uint16_t code = this->va_cio_type1.getButtonCode(btn);
        
        // Inject the code into the driver for the next packet
        this->va_cio_type1.setButtonCode(code);
        
        // Simple debounce/timing: only pop one button per loop cycle
        this->button_queue_.pop();
        ESP_LOGD(TAG, "Injected button code: 0x%04X", code);
    } else {
        // Reset to NOBTN code (0x1B1B) if nothing is queued
        this->va_cio_type1.setButtonCode(this->va_cio_type1.getButtonCode(bestway_va::NOBTN));
    }
  }
}

void BestwaySpa::control(const climate::ClimateCall &call) {
  if (call.get_target_temperature().has_value()) {
    float temp = *call.get_target_temperature();
    // In the future, we can add logic here to compare 'temp' to current target
    // and queue UP/DOWN presses automatically.
    ESP_LOGD(TAG, "Target temperature change requested: %.1f", temp);
  }
}

void BestwaySpa::on_button_press_(bestway_va::Buttons button) {
  ESP_LOGD(TAG, "Button press queued: %d", (uint8_t)button);
  this->button_queue_.push(button);
}

void BestwaySpa::dump_config() {
  LOG_CLIMATE("", "Bestway Spa", this);
  ESP_LOGCONFIG(TAG, "  Protocol: %s", (this->protocol_type_ == PROTOCOL_6WIRE_T1) ? "6-WIRE TYPE 1" : "OTHER");
  ESP_LOGCONFIG(TAG, "  DSP MITM Enabled: %s", this->dsp_enabled_ ? "YES" : "NO");
}

}  // namespace bestway_spa
}  // namespace esphome
