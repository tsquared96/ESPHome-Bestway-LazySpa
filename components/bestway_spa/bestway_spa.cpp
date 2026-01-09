#include "bestway_spa.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bestway_spa {

static const char *const TAG = "bestway_spa";

void BestwaySpa::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Bestway Spa...");

  if (protocol_type_ == PROTOCOL_6WIRE_T1) {
    int clk = cio_clk_pin_->get_pin();
    int data = cio_data_pin_->get_pin();
    int cs = cio_cs_pin_->get_pin();
    
    // Initialize Pump-side Driver
    va_cio_type1.setup(data, clk, cs);
    
    // Initialize Display-side Driver (if pins are configured)
    if (dsp_clk_pin_ != nullptr) {
        int d_clk = dsp_clk_pin_->get_pin();
        int d_data = dsp_data_pin_->get_pin();
        int d_cs = dsp_cs_pin_->get_pin();
        int d_audio = (audio_pin_ != nullptr) ? audio_pin_->get_pin() : -1;
        
        va_dsp_type1.setup(d_data, d_clk, d_cs, d_audio);
        dsp_enabled_ = true;
    }

    current_button_code_ = va_cio_type1.getButtonCode(bestway_va::NOBTN);
    ESP_LOGI(TAG, "6-Wire Protocol Initialized (160MHz Mode)");
  }
}

void BestwaySpa::loop() {
  if (protocol_type_ == PROTOCOL_6WIRE_T1) {
    handle_6wire_protocol_();
  } else {
    // Fallback for 4-wire UART if needed
    handle_4wire_protocol_();
  }
}

void BestwaySpa::handle_6wire_protocol_() {
  // 1. Get latest data from the Pump
  va_cio_type1.updateStates();

  // 2. Relay to physical display if enabled
  if (dsp_enabled_) {
    va_dsp_type1.dsp_states = va_cio_type1.cio_states;
    va_dsp_type1.handleStates();

    // Read physical buttons from the tub panel
    bestway_va::Buttons physical_btn = va_dsp_type1.getPressedButton();
    if (physical_btn != bestway_va::NOBTN && physical_btn != last_physical_btn_) {
      this->on_button_press_(static_cast<Buttons>(physical_btn));
      last_physical_btn_ = physical_btn;
    } else if (physical_btn == bestway_va::NOBTN) {
      last_physical_btn_ = bestway_va::NOBTN;
    }
  }

  // 3. Update ESPHome state if a new valid packet arrived
  if (va_cio_type1.good_packets_count > last_pkt_count_) {
    last_pkt_count_ = va_cio_type1.good_packets_count;

    state_.current_temp = (float)va_cio_type1.cio_states.temperature;
    state_.target_temp = (float)va_cio_type1.cio_states.target_temp;
    state_.power = va_cio_type1.cio_states.power;
    state_.heater_enabled = va_cio_type1.cio_states.heat;
    state_.filter_pump = va_cio_type1.cio_states.pump;
    state_.bubbles = va_cio_type1.cio_states.bubbles;
    state_.locked = va_cio_type1.cio_states.locked;
    
    // Fix for your compiler error: access the now-public brightness
    state_.brightness = va_cio_type1.brightness;

    // Sync climate properties
    this->current_temperature = state_.current_temp;
    this->target_temperature = state_.target_temp;
    
    // Limit publishing frequency to HA to save CPU
    uint32_t now = millis();
    if (now - last_update_ > 5000) {
      this->publish_state();
      last_update_ = now;
    }
  }

  // 4. Handle Outgoing Commands (ESPHome -> Pump)
  process_button_queue_();
  va_cio_type1.setButtonCode(current_button_code_);
}

void BestwaySpa::process_button_queue_() {
  if (button_queue_.empty()) {
    current_button_code_ = va_cio_type1.getButtonCode(bestway_va::NOBTN);
    return;
  }

  uint32_t now = millis();
  // Provide enough time for the pump to register the "press"
  if (now - last_button_time_ > 400) {
    Buttons btn = button_queue_.front();
    button_queue_.pop();
    current_button_code_ = va_cio_type1.getButtonCode(static_cast<bestway_va::Buttons>(btn));
    last_button_time_ = now;
  }
}

// Boilerplate for Climate trait control
void BestwaySpa::control(const climate::ClimateCall &call) {
  if (call.get_target_temperature().has_value()) {
    float temp = *call.get_target_temperature();
    this->set_target_temperature_(temp);
  }
  // Add other mode changes here if needed
}

void BestwaySpa::on_button_press_(Buttons btn) {
  button_queue_.push(btn);
  ESP_LOGD(TAG, "Button queued: %d", btn);
}

} // namespace bestway_spa
} // namespace esphome
