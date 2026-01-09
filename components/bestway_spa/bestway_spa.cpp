#include "bestway_spa.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bestway_spa {

static const char *const TAG = "bestway_spa";

void BestwaySpa::setup() {
  if (protocol_type_ == PROTOCOL_6WIRE_T1) {
    va_cio_type1.setup(cio_data_pin_->get_pin(), cio_clk_pin_->get_pin(), cio_cs_pin_->get_pin());
    
    if (dsp_clk_pin_ != nullptr) {
        int audio = (audio_pin_ != nullptr) ? audio_pin_->get_pin() : -1;
        va_dsp_type1.setup(dsp_data_pin_->get_pin(), dsp_clk_pin_->get_pin(), dsp_cs_pin_->get_pin(), audio);
        dsp_enabled_ = true;
    }
    current_button_code_ = va_cio_type1.getButtonCode(bestway_va::NOBTN);
  }
}

void BestwaySpa::loop() {
  if (protocol_type_ == PROTOCOL_6WIRE_T1) {
    handle_6wire_protocol_();
  }
}

void BestwaySpa::handle_6wire_protocol_() {
  va_cio_type1.updateStates();

  if (dsp_enabled_) {
    va_dsp_type1.dsp_states = va_cio_type1.cio_states;
    va_dsp_type1.handleStates();

    bestway_va::Buttons physical_btn = va_dsp_type1.getPressedButton();
    if (physical_btn != bestway_va::NOBTN && physical_btn != last_physical_btn_) {
      this->on_button_press_(static_cast<Buttons>(physical_btn));
      last_physical_btn_ = physical_btn;
    } else if (physical_btn == bestway_va::NOBTN) {
      last_physical_btn_ = bestway_va::NOBTN;
    }
  }

  if (va_cio_type1.good_packets_count > last_pkt_count_) {
    last_pkt_count_ = va_cio_type1.good_packets_count;

    state_.current_temp = (float)va_cio_type1.cio_states.temperature;
    state_.power = va_cio_type1.cio_states.power;
    state_.heater_enabled = va_cio_type1.cio_states.heat;
    state_.filter_pump = va_cio_type1.cio_states.pump;
    state_.bubbles = va_cio_type1.cio_states.bubbles;
    state_.locked = va_cio_type1.cio_states.locked;
    state_.brightness = va_cio_type1.brightness;

    this->current_temperature = state_.current_temp;
    
    uint32_t now = millis();
    if (now - last_update_ > 5000) {
      this->publish_state();
      last_update_ = now;
    }
  }

  process_button_queue_();
  va_cio_type1.setButtonCode(current_button_code_);
}

void BestwaySpa::process_button_queue_() {
  if (button_queue_.empty()) {
    current_button_code_ = va_cio_type1.getButtonCode(bestway_va::NOBTN);
    return;
  }

  uint32_t now = millis();
  if (now - last_button_time_ > 400) {
    Buttons btn = button_queue_.front();
    button_queue_.pop();
    current_button_code_ = va_cio_type1.getButtonCode(static_cast<bestway_va::Buttons>(btn));
    last_button_time_ = now;
  }
}

void BestwaySpa::on_button_press_(Buttons btn) {
  button_queue_.push(btn);
}

climate::ClimateTraits BestwaySpa::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_visual_min_temperature(20);
  traits.set_visual_max_temperature(40);
  traits.set_visual_temperature_step(1);
  return traits;
}

void BestwaySpa::control(const climate::ClimateCall &call) {
  if (call.get_target_temperature().has_value()) {
    this->target_temperature = *call.get_target_temperature();
    this->publish_state();
  }
}

void BestwaySpa::dump_config() {
    ESP_LOGCONFIG(TAG, "Bestway Spa:");
    LOG_PIN("  CIO Data Pin: ", cio_data_pin_);
    LOG_PIN("  CIO Clock Pin: ", cio_clk_pin_);
    LOG_PIN("  CIO CS Pin: ", cio_cs_pin_);
}

} // namespace bestway_spa
} // namespace esphome
