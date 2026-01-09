#include "bestway_spa.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bestway_spa {

static const char *const TAG = "bestway_spa";

BestwaySpa *BestwaySpa::instance = nullptr;

void IRAM_ATTR BestwaySpa::cio_clk_intr() { instance->va_cio_type1.isr_clkHandler(); }
void IRAM_ATTR BestwaySpa::cio_cs_intr()  { instance->va_cio_type1.isr_packetHandler(); }
void IRAM_ATTR BestwaySpa::dsp_clk_intr() { instance->va_dsp_type1.isr_clkHandler(); }
void IRAM_ATTR BestwaySpa::dsp_cs_intr()  { instance->va_dsp_type1.isr_packetHandler(); }

void BestwaySpa::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Bestway Spa...");
  instance = this; 

  if (this->protocol_type_ == PROTOCOL_6WIRE_T1) {
    this->va_cio_type1.setup(
      this->cio_data_pin_->get_pin(), 
      this->cio_clk_pin_->get_pin(), 
      this->cio_cs_pin_->get_pin()
    );

    this->cio_clk_pin_->setup();
    this->cio_cs_pin_->setup();
    this->cio_data_pin_->setup();

    attachInterrupt(this->cio_clk_pin_->get_pin(), cio_clk_intr, CHANGE);
    attachInterrupt(this->cio_cs_pin_->get_pin(), cio_cs_intr, CHANGE);

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

      attachInterrupt(this->dsp_clk_pin_->get_pin(), dsp_clk_intr, CHANGE);
      attachInterrupt(this->dsp_cs_pin_->get_pin(), dsp_cs_intr, CHANGE);
      
      this->dsp_enabled_ = true;
    }
  }
}

void BestwaySpa::loop() {
  if (this->protocol_type_ == PROTOCOL_6WIRE_T1) {
    this->va_cio_type1.updateStates();
    
    if (this->dsp_enabled_) {
      this->va_dsp_type1.handleStates();
      
      if (this->display_text_sensor_ != nullptr) {
        std::string current_text = this->va_dsp_type1.dsp_states.display_text;
        if (this->display_text_sensor_->state != current_text) {
          this->display_text_sensor_->publish_state(current_text);
        }
      }
    }

    float cur_temp = (float)this->va_cio_type1.cio_states.temperature;
    float tar_temp = (float)this->va_cio_type1.cio_states.target;

    this->current_temperature = cur_temp;
    this->target_temperature = tar_temp;

    if (this->current_temp_sensor_ != nullptr && this->current_temp_sensor_->state != cur_temp)
        this->current_temp_sensor_->publish_state(cur_temp);
    
    if (this->target_temp_sensor_ != nullptr && this->target_temp_sensor_->state != tar_temp)
        this->target_temp_sensor_->publish_state(tar_temp);

    if (this->heating_sensor_ != nullptr)
        this->heating_sensor_->publish_state(this->va_cio_type1.cio_states.heater);
    
    if (this->filter_sensor_ != nullptr)
        this->filter_sensor_->publish_state(this->va_cio_type1.cio_states.filter);

    if (this->bubbles_sensor_ != nullptr)
        this->bubbles_sensor_->publish_state(this->va_cio_type1.cio_states.bubbles);

    if (!this->button_queue_.empty()) {
        Buttons btn = this->button_queue_.front();
        uint16_t code = this->va_cio_type1.getButtonCode(btn);
        this->va_cio_type1.setButtonCode(code);
        this->button_queue_.pop();
        ESP_LOGD(TAG, "Injected button: %d (0x%04X)", btn, code);
    } else {
        this->va_cio_type1.setButtonCode(this->va_cio_type1.getButtonCode(bestway_va::NOBTN));
    }
  }
}

climate::ClimateTraits BestwaySpa::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_supported_feature(climate::CLIMATE_SUPPORT_CURRENT_TEMPERATURE);
  traits.set_visual_min_temperature(20);
  traits.set_visual_max_temperature(40);
  traits.set_visual_temperature_step(1);
  return traits;
}

void BestwaySpa::control(const climate::ClimateCall &call) {
  if (call.get_target_temperature().has_value()) {
    float new_target = *call.get_target_temperature();
    float current_target = (float)this->va_cio_type1.cio_states.target;
    
    if (new_target > current_target) {
        this->on_button_press_(bestway_va::TEMP_UP);
    } else if (new_target < current_target) {
        this->on_button_press_(bestway_va::TEMP_DOWN);
    }
  }
}

void BestwaySpa::on_button_press_(Buttons button) {
  this->button_queue_.push(button);
}

void BestwaySpa::dump_config() {
  LOG_CLIMATE("", "Bestway Spa", this);
}

}  // namespace bestway_spa
}  // namespace esphome
