#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "cio_type1.h"
#include "dsp_type1.h"
#include <vector>

namespace esphome {
namespace bestway_spa {

// =============================================================================
// ENUMERATIONS
// =============================================================================

// Protocol types
enum ProtocolType {
  PROTOCOL_4WIRE,     // 2021+ models (UART)
  PROTOCOL_6WIRE_T1,  // Pre-2021 6-wire TYPE1 (SPI-like)
  PROTOCOL_6WIRE_T2   // 6-wire TYPE2 (54149E)
};

// Spa model identifiers
enum SpaModel {
  MODEL_PRE2021,    // Pre-2021 (6-wire TYPE1)
  MODEL_54149E,     // 6-wire TYPE2
  MODEL_54123,      // 4-wire (NO54123)
  MODEL_54138,      // 4-wire with jets and air
  MODEL_54144,      // 4-wire with jets
  MODEL_54154,      // 4-wire
  MODEL_54173,      // 4-wire with jets and air
  MODEL_P05504,     // 6-wire TYPE1 variant
  MODEL_UNKNOWN
};

// =============================================================================
// DATA STRUCTURES
// =============================================================================

// Spa state tracking
struct SpaState {
  bool locked = false;
  bool power = true;
  bool heater_enabled = true;
  bool heater_green = false;     // Ready to heat
  bool heater_red = false;       // Actively heating
  bool filter_pump = false;
  bool bubbles = false;
  bool jets = false;
  bool unit_celsius = true;
  bool timer_active = false;
  uint8_t timer_hours = 0;
  uint8_t error_code = 0;

  float current_temp = 20.0f;
  float target_temp = 37.0f;

  uint8_t brightness = 8;

  // Raw display characters
  char display_chars[4] = {' ', ' ', ' ', '\0'};
};

// 4-wire model configurations
struct ModelConfig4W {
  uint8_t heat_bitmask1;
  uint8_t heat_bitmask2;
  uint8_t pump_bitmask;
  uint8_t bubbles_bitmask;
  uint8_t jets_bitmask;
  bool has_jets;
  bool has_air;
};

// Model configurations
static const ModelConfig4W CONFIG_54123 = {0x02, 0x08, 0x04, 0x10, 0x00, false, false};
static const ModelConfig4W CONFIG_54138 = {0x30, 0x40, 0x04, 0x08, 0x80, true, true};
static const ModelConfig4W CONFIG_54144 = {0x30, 0x40, 0x04, 0x08, 0x80, true, false};
static const ModelConfig4W CONFIG_54154 = {0x02, 0x08, 0x04, 0x10, 0x00, false, false};
static const ModelConfig4W CONFIG_54173 = {0x30, 0x40, 0x04, 0x08, 0x80, true, true};

// Button codes for PRE2021 (6-wire TYPE1)
static const uint16_t BTN_CODES_PRE2021[] = {
  0x1B1B, 0x0200, 0x0100, 0x0300, 0x1012, 0x1212, 0x1112, 0x1312, 0x0809, 0x0000, 0x0000
};

// Button codes for P05504 (6-wire TYPE1 variant)
static const uint16_t BTN_CODES_P05504[] = {
  0x1B1B, 0x0210, 0x0110, 0x0310, 0x1022, 0x1222, 0x1122, 0x1322, 0x081A, 0x0000, 0x0000
};

// =============================================================================
// MAIN SPA CLASS
// =============================================================================

class BestwaySpa : public climate::Climate, public uart::UARTDevice, public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // Climate interface
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  // Configuration setters
  void set_protocol_type(ProtocolType type) { protocol_type_ = type; }
  void set_model(SpaModel model) { model_ = model; }

  // 6-wire pin configuration (CIO side - receives data FROM CIO)
  void set_clk_pin(InternalGPIOPin *pin) { cio_clk_pin_ = pin; }
  void set_data_pin(InternalGPIOPin *pin) { cio_data_pin_ = pin; }
  void set_cs_pin(InternalGPIOPin *pin) { cio_cs_pin_ = pin; }

  // DSP side pins (optional - if different from CIO)
  void set_dsp_clk_pin(InternalGPIOPin *pin) { dsp_clk_pin_ = pin; }
  void set_dsp_data_pin(InternalGPIOPin *pin) { dsp_data_pin_ = pin; }
  void set_dsp_cs_pin(InternalGPIOPin *pin) { dsp_cs_pin_ = pin; }
  void set_audio_pin(InternalGPIOPin *pin) { audio_pin_ = pin; }

  // Sensors
  void set_current_temperature_sensor(sensor::Sensor *sensor) { current_temp_sensor_ = sensor; }
  void set_target_temperature_sensor(sensor::Sensor *sensor) { target_temp_sensor_ = sensor; }

  // Binary sensors
  void set_heating_sensor(binary_sensor::BinarySensor *sensor) { heating_sensor_ = sensor; }
  void set_filter_sensor(binary_sensor::BinarySensor *sensor) { filter_sensor_ = sensor; }
  void set_bubbles_sensor(binary_sensor::BinarySensor *sensor) { bubbles_sensor_ = sensor; }
  void set_jets_sensor(binary_sensor::BinarySensor *sensor) { jets_sensor_ = sensor; }
  void set_locked_sensor(binary_sensor::BinarySensor *sensor) { locked_sensor_ = sensor; }
  void set_power_sensor(binary_sensor::BinarySensor *sensor) { power_sensor_ = sensor; }
  void set_error_sensor(binary_sensor::BinarySensor *sensor) { error_sensor_ = sensor; }

  // Text sensors
  void set_error_text_sensor(text_sensor::TextSensor *sensor) { error_text_sensor_ = sensor; }
  void set_display_text_sensor(text_sensor::TextSensor *sensor) { display_text_sensor_ = sensor; }

  // Control methods (called by switches and automation)
  void set_power(bool state);
  void set_heater(bool state);
  void set_filter(bool state);
  void set_bubbles(bool state);
  void set_jets(bool state);
  void set_lock(bool state);
  void set_unit(bool celsius);
  void set_target_temp(float temp);
  void adjust_target_temp(int8_t delta);
  void set_timer(uint8_t hours);
  void set_brightness(uint8_t level);

  // State getters
  const SpaState& get_state() const { return state_; }
  bool has_jets() const;
  bool has_air() const;
  SpaModel get_model() const { return model_; }

 protected:
  // Protocol handlers
  void handle_4wire_protocol_();
  void handle_6wire_type1_protocol_();
  void handle_6wire_type2_protocol_();

  // State management
  void sync_cio_to_dsp_();
  void handle_button_press_(Buttons button);
  void update_climate_state_();
  void update_sensors_();

  // 4-wire packet handling
  void parse_4wire_packet_(const std::vector<uint8_t> &packet);
  void send_4wire_response_();

  // Utilities
  float celsius_to_fahrenheit_(float c) { return c * 9.0f / 5.0f + 32.0f; }
  float fahrenheit_to_celsius_(float f) { return (f - 32.0f) * 5.0f / 9.0f; }

  // Configuration
  ProtocolType protocol_type_{PROTOCOL_4WIRE};
  SpaModel model_{MODEL_54154};

  // GPIO pins for 6-wire CIO side
  InternalGPIOPin *cio_clk_pin_{nullptr};
  InternalGPIOPin *cio_data_pin_{nullptr};
  InternalGPIOPin *cio_cs_pin_{nullptr};

  // GPIO pins for 6-wire DSP side (may be same as CIO for man-in-middle)
  InternalGPIOPin *dsp_clk_pin_{nullptr};
  InternalGPIOPin *dsp_data_pin_{nullptr};
  InternalGPIOPin *dsp_cs_pin_{nullptr};
  InternalGPIOPin *audio_pin_{nullptr};

  // 6-wire handlers
  CIO_TYPE1 cio_type1_;
  DSP_TYPE1 dsp_type1_;

  // State
  SpaState state_;

  // Sensors
  sensor::Sensor *current_temp_sensor_{nullptr};
  sensor::Sensor *target_temp_sensor_{nullptr};
  binary_sensor::BinarySensor *heating_sensor_{nullptr};
  binary_sensor::BinarySensor *filter_sensor_{nullptr};
  binary_sensor::BinarySensor *bubbles_sensor_{nullptr};
  binary_sensor::BinarySensor *jets_sensor_{nullptr};
  binary_sensor::BinarySensor *locked_sensor_{nullptr};
  binary_sensor::BinarySensor *power_sensor_{nullptr};
  binary_sensor::BinarySensor *error_sensor_{nullptr};
  text_sensor::TextSensor *error_text_sensor_{nullptr};
  text_sensor::TextSensor *display_text_sensor_{nullptr};

  // 4-wire state
  std::vector<uint8_t> rx_buffer_;
  uint32_t last_packet_time_{0};
  uint32_t last_state_update_{0};
  uint32_t last_sensor_update_{0};
  bool waiting_for_response_{false};
  uint8_t heater_stage_{0};
  uint32_t stage_start_time_{0};
  const ModelConfig4W *model_config_{&CONFIG_54154};

  // Button handling state
  Buttons pending_button_{NOBTN};
  int temp_adjust_steps_{0};
  bool power_on_pending_{false};

  // 6-wire DSP state
  bool dsp_initialized_{false};
};

// =============================================================================
// SWITCH IMPLEMENTATION
// =============================================================================

enum SwitchType {
  SWITCH_HEATER,
  SWITCH_FILTER,
  SWITCH_BUBBLES,
  SWITCH_JETS,
  SWITCH_LOCK,
  SWITCH_POWER
};

class BestwaySpaSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void set_switch_type(SwitchType type) { type_ = type; }

  void write_state(bool state) override {
    if (parent_ == nullptr) return;

    switch (type_) {
      case SWITCH_HEATER:
        parent_->set_heater(state);
        break;
      case SWITCH_FILTER:
        parent_->set_filter(state);
        break;
      case SWITCH_BUBBLES:
        parent_->set_bubbles(state);
        break;
      case SWITCH_JETS:
        if (parent_->has_jets()) {
          parent_->set_jets(state);
        }
        break;
      case SWITCH_LOCK:
        parent_->set_lock(state);
        break;
      case SWITCH_POWER:
        parent_->set_power(state);
        break;
    }
    publish_state(state);
  }

 protected:
  BestwaySpa *parent_{nullptr};
  SwitchType type_{SWITCH_HEATER};
};

}  // namespace bestway_spa
}  // namespace esphome
