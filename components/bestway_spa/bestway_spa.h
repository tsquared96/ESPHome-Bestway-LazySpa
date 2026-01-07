#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include <vector>

namespace esphome {
namespace bestway_spa {

// =============================================================================
// ENUMERATIONS - Based on VisualApproach firmware
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

// Button indices - matches VisualApproach
enum Buttons : uint8_t {
  NOBTN = 0,
  LOCK,
  TIMER,
  BUBBLES,
  UNIT,
  HEAT,
  PUMP,
  DOWN,
  UP,
  POWER,
  HYDROJETS,
  BTN_COUNT
};

// State indices - matches VisualApproach
enum States : uint8_t {
  LOCKEDSTATE = 0,
  POWERSTATE,
  UNITSTATE,
  BUBBLESSTATE,
  HEATGRNSTATE,
  HEATREDSTATE,
  HEATSTATE,
  PUMPSTATE,
  CHAR1,
  CHAR2,
  CHAR3,
  JETSSTATE,
  GODSTATE,
  TIMERSTATE,
  STATE_COUNT
};

// =============================================================================
// DATA STRUCTURES
// =============================================================================

// Spa state tracking
struct SpaState {
  bool locked = false;
  bool power = true;
  bool heater_enabled = false;
  bool heater_green = false;     // Ready to heat
  bool heater_red = false;       // Actively heating
  bool filter_pump = false;
  bool bubbles = false;
  bool jets = false;
  bool unit_celsius = true;
  bool timer_active = false;
  uint8_t timer_hours = 0;
  uint8_t error_code = 0;

  float current_temp = 20.0;
  float target_temp = 37.0;

  uint8_t brightness = 8;

  // Raw display characters
  char display_chars[4] = {' ', ' ', ' ', '\0'};
};

// Toggle requests
struct SpaToggles {
  bool power_pressed = false;
  bool lock_pressed = false;
  bool timer_pressed = false;
  bool bubbles_pressed = false;
  bool jets_pressed = false;
  bool heat_pressed = false;
  bool pump_pressed = false;
  bool up_pressed = false;
  bool down_pressed = false;
  bool unit_pressed = false;

  bool set_target_temp = false;
  int8_t target_temp_delta = 0;
};

// Button queue item for 6-wire protocol
struct ButtonQueueItem {
  uint16_t button_code;
  uint8_t target_state;
  int target_value;
  int duration_ms;
  uint32_t start_time;
};

// =============================================================================
// 7-SEGMENT DISPLAY CHARACTER CODES
// =============================================================================

// 7-segment character codes for TYPE1 (38 characters)
static const uint8_t CHARCODES_TYPE1[] = {
  0x7F, 0x0D, 0xB7, 0x9F, 0xCD, 0xDB, 0xFB, 0x0F, 0xFF, 0xDF,  // 0-9
  0xEF, 0xF9, 0x73, 0xBD, 0xF3, 0xE3, 0x7B, 0xE9, 0x09, 0x3D,  // A-J
  0xE1, 0x71, 0x49, 0xA9, 0xB9, 0xE7, 0xCF, 0xA1, 0xDB, 0xF1,  // K-T
  0x7D, 0x7D, 0x7D, 0xED, 0xDD, 0xB7, 0x00, 0x80              // U-Z, space, dash
};

// 7-segment character codes for TYPE2
static const uint8_t CHARCODES_TYPE2[] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F,  // 0-9
  0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71, 0x3D, 0x76, 0x06, 0x0E,  // A-J
  0x70, 0x38, 0x15, 0x54, 0x5C, 0x73, 0x67, 0x50, 0x6D, 0x78,  // K-T
  0x3E, 0x3E, 0x3E, 0x76, 0x6E, 0x5B, 0x00, 0x40              // U-Z, space, dash
};

// Character lookup table
static const char CHARS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ -";

// =============================================================================
// BUTTON CODES BY MODEL
// =============================================================================

// Button codes for PRE2021 (6-wire TYPE1)
// Index: NOBTN, LOCK, TIMER, BUBBLES, UNIT, HEAT, PUMP, DOWN, UP, POWER, JETS
static const uint16_t BTN_CODES_PRE2021[] = {
  0x1B1B, 0x0200, 0x0100, 0x0300, 0x1012, 0x1212, 0x1112, 0x1312, 0x0809, 0x0000, 0x0000
};

// Button codes for P05504 (6-wire TYPE1 variant)
static const uint16_t BTN_CODES_P05504[] = {
  0x1B1B, 0x0210, 0x0110, 0x0310, 0x1022, 0x1222, 0x1122, 0x1322, 0x081A, 0x0000, 0x0000
};

// Button codes for MODEL54149E (6-wire TYPE2)
static const uint16_t BTN_CODES_54149E[] = {
  0x0000, 0x0080, 0x0040, 0x0020, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001, 0x0100, 0x0200
};

// =============================================================================
// 4-WIRE MODEL CONFIGURATIONS
// =============================================================================

// Heater bitmasks for 4-wire models
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

// =============================================================================
// MAIN SPA CLASS
// =============================================================================

class BestwaySpa;

// Forward declarations for switches
class BestwaySpaHeaterSwitch;
class BestwaySpaFilterSwitch;
class BestwaySpaBubblesSwitch;
class BestwaySpaJetsSwitch;
class BestwaySpaLockSwitch;
class BestwaySpaPowerSwitch;

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

  // 6-wire pin configuration - MITM dual-bus architecture
  // CIO bus pins (from pump controller to ESP - input)
  void set_cio_data_pin(InternalGPIOPin *pin) { cio_data_pin_ = pin; }
  void set_cio_clk_pin(InternalGPIOPin *pin) { cio_clk_pin_ = pin; }
  void set_cio_cs_pin(InternalGPIOPin *pin) { cio_cs_pin_ = pin; }
  // DSP bus pins (from ESP to physical display panel - output)
  void set_dsp_data_pin(InternalGPIOPin *pin) { dsp_data_pin_ = pin; }
  void set_dsp_clk_pin(InternalGPIOPin *pin) { dsp_clk_pin_ = pin; }
  void set_dsp_cs_pin(InternalGPIOPin *pin) { dsp_cs_pin_ = pin; }
  void set_audio_pin(InternalGPIOPin *pin) { audio_pin_ = pin; }

  // Legacy single-bus setters (for backwards compatibility)
  void set_clk_pin(InternalGPIOPin *pin) { cio_clk_pin_ = pin; }
  void set_data_pin(InternalGPIOPin *pin) { cio_data_pin_ = pin; }
  void set_cs_pin(InternalGPIOPin *pin) { cio_cs_pin_ = pin; }

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
  void set_button_status_sensor(text_sensor::TextSensor *sensor) { button_status_sensor_ = sensor; }

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

  // Button enable/disable control
  void set_button_enabled(Buttons btn, bool enabled);
  bool is_button_enabled(Buttons btn) const;

  // State getters
  const SpaState& get_state() const { return state_; }
  bool has_jets() const;
  bool has_air() const;
  SpaModel get_model() const { return model_; }

  // Data pin access for ISR (must be public for ISR callback)
  bool read_data_pin_();
  void write_data_pin_(bool value);

 protected:
  // Protocol handlers
  void handle_4wire_protocol_();
  void handle_6wire_protocol_();  // Interrupt-driven 6-wire handler

  // 6-wire packet handling now done by VA's updateStates()

  // 4-wire packet handling
  void parse_4wire_packet_(const std::vector<uint8_t> &packet);
  void send_4wire_response_();

  // State management
  void update_states_from_payload_();
  void handle_toggles_();
  void update_climate_state_();
  void update_sensors_();

  // Button queue for 6-wire
  void queue_button_(Buttons button, int duration_ms = 300);
  void process_button_queue_();
  uint16_t get_button_code_(Buttons button);

  // Character decoding now done by VA's _getChar() method

  // Utilities
  float celsius_to_fahrenheit_(float c) { return c * 9.0f / 5.0f + 32.0f; }
  float fahrenheit_to_celsius_(float f) { return (f - 32.0f) * 5.0f / 9.0f; }
  uint8_t calculate_checksum_(const uint8_t *data, size_t len);

  // Configuration
  ProtocolType protocol_type_{PROTOCOL_4WIRE};
  SpaModel model_{MODEL_54154};

  // GPIO pins for 6-wire MITM dual-bus architecture
  // CIO bus (input from pump controller)
  InternalGPIOPin *cio_clk_pin_{nullptr};
  InternalGPIOPin *cio_data_pin_{nullptr};
  InternalGPIOPin *cio_cs_pin_{nullptr};
  // DSP bus (output to physical display panel)
  InternalGPIOPin *dsp_data_pin_{nullptr};
  InternalGPIOPin *dsp_clk_pin_{nullptr};
  InternalGPIOPin *dsp_cs_pin_{nullptr};
  InternalGPIOPin *audio_pin_{nullptr};

  // State
  SpaState state_;
  SpaToggles toggles_;

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
  text_sensor::TextSensor *button_status_sensor_{nullptr};

  // Packet buffers
  std::vector<uint8_t> rx_buffer_;
  uint8_t cio_payload_[16]{0};
  uint8_t dsp_payload_[16]{0};
  size_t cio_payload_len_{0};
  size_t dsp_payload_len_{0};

  // Timing
  uint32_t last_packet_time_{0};
  uint32_t last_state_update_{0};
  uint32_t last_sensor_update_{0};
  uint32_t last_stats_time_{0};

  // Statistics (for 6-wire interrupt-driven)
  uint32_t good_packets_{0};
  uint32_t last_cs_count_{0};
  uint32_t last_clk_count_{0};
  uint32_t last_pkt_count_{0};

  // Button queue
  std::vector<ButtonQueueItem> button_queue_;
  uint16_t current_button_code_{0};
  bool button_enabled_[BTN_COUNT]{true, true, true, true, true, true, true, true, true, true, true};

  // Protocol state
  bool paused_{false};
  bool new_packet_available_{false};
  uint8_t bit_counter_{0};
  uint8_t byte_buffer_{0};

  // State debouncing - require consistent readings before updating
  SpaState pending_state_;
  uint8_t state_match_count_{0};
  static const uint8_t STATE_DEBOUNCE_COUNT = 3;  // Require 3 matching packets

  // 4-wire state
  bool waiting_for_response_{false};
  uint8_t heater_stage_{0};
  uint32_t stage_start_time_{0};

  // Model config
  const ModelConfig4W *model_config_{&CONFIG_54154};
};

// =============================================================================
// SWITCH IMPLEMENTATIONS
// =============================================================================

class BestwaySpaHeaterSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void write_state(bool state) override {
    parent_->set_heater(state);
    publish_state(state);
  }
 protected:
  BestwaySpa *parent_{nullptr};
};

class BestwaySpaFilterSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void write_state(bool state) override {
    parent_->set_filter(state);
    publish_state(state);
  }
 protected:
  BestwaySpa *parent_{nullptr};
};

class BestwaySpaBubblesSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void write_state(bool state) override {
    parent_->set_bubbles(state);
    publish_state(state);
  }
 protected:
  BestwaySpa *parent_{nullptr};
};

class BestwaySpaJetsSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void write_state(bool state) override {
    if (parent_->has_jets()) {
      parent_->set_jets(state);
      publish_state(state);
    }
  }
 protected:
  BestwaySpa *parent_{nullptr};
};

class BestwaySpaLockSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void write_state(bool state) override {
    parent_->set_lock(state);
    publish_state(state);
  }
 protected:
  BestwaySpa *parent_{nullptr};
};

class BestwaySpaPowerSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void write_state(bool state) override {
    parent_->set_power(state);
    publish_state(state);
  }
 protected:
  BestwaySpa *parent_{nullptr};
};

class BestwaySpaUnitSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void write_state(bool state) override {
    // state: true = Celsius, false = Fahrenheit
    parent_->set_unit(state);
    publish_state(state);
  }
 protected:
  BestwaySpa *parent_{nullptr};
};

class BestwaySpaTimerSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void write_state(bool state) override {
    parent_->set_timer(state ? 1 : 0);  // Toggle timer
    publish_state(state);
  }
 protected:
  BestwaySpa *parent_{nullptr};
};

// Button switches - for triggering single button presses
class BestwaySpaUpButton : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void write_state(bool state) override {
    if (state) {
      parent_->adjust_target_temp(1);  // +1 degree
    }
    publish_state(false);  // Momentary - always return to off
  }
 protected:
  BestwaySpa *parent_{nullptr};
};

class BestwaySpaDownButton : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void write_state(bool state) override {
    if (state) {
      parent_->adjust_target_temp(-1);  // -1 degree
    }
    publish_state(false);  // Momentary - always return to off
  }
 protected:
  BestwaySpa *parent_{nullptr};
};

// =============================================================================
// BUTTON ENABLE/DISABLE SWITCHES
// =============================================================================
// These switches allow enabling/disabling individual buttons from Home Assistant

class BestwaySpaHeaterBtnEnabled : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void setup() override { publish_state(true); }  // Default: enabled
  void write_state(bool state) override {
    parent_->set_button_enabled(HEAT, state);
    publish_state(state);
  }
 protected:
  BestwaySpa *parent_{nullptr};
};

class BestwaySpaFilterBtnEnabled : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void setup() override { publish_state(true); }
  void write_state(bool state) override {
    parent_->set_button_enabled(PUMP, state);
    publish_state(state);
  }
 protected:
  BestwaySpa *parent_{nullptr};
};

class BestwaySpaBubblesBtnEnabled : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void setup() override { publish_state(true); }
  void write_state(bool state) override {
    parent_->set_button_enabled(BUBBLES, state);
    publish_state(state);
  }
 protected:
  BestwaySpa *parent_{nullptr};
};

class BestwaySpaJetsBtnEnabled : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void setup() override { publish_state(true); }
  void write_state(bool state) override {
    parent_->set_button_enabled(HYDROJETS, state);
    publish_state(state);
  }
 protected:
  BestwaySpa *parent_{nullptr};
};

class BestwaySpaLockBtnEnabled : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void setup() override { publish_state(true); }
  void write_state(bool state) override {
    parent_->set_button_enabled(LOCK, state);
    publish_state(state);
  }
 protected:
  BestwaySpa *parent_{nullptr};
};

class BestwaySpaPowerBtnEnabled : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void setup() override { publish_state(true); }
  void write_state(bool state) override {
    parent_->set_button_enabled(POWER, state);
    publish_state(state);
  }
 protected:
  BestwaySpa *parent_{nullptr};
};

class BestwaySpaUnitBtnEnabled : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void setup() override { publish_state(true); }
  void write_state(bool state) override {
    parent_->set_button_enabled(UNIT, state);
    publish_state(state);
  }
 protected:
  BestwaySpa *parent_{nullptr};
};

class BestwaySpaTimerBtnEnabled : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void setup() override { publish_state(true); }
  void write_state(bool state) override {
    parent_->set_button_enabled(TIMER, state);
    publish_state(state);
  }
 protected:
  BestwaySpa *parent_{nullptr};
};

class BestwaySpaUpBtnEnabled : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void setup() override { publish_state(true); }
  void write_state(bool state) override {
    parent_->set_button_enabled(UP, state);
    publish_state(state);
  }
 protected:
  BestwaySpa *parent_{nullptr};
};

class BestwaySpaDownBtnEnabled : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void setup() override { publish_state(true); }
  void write_state(bool state) override {
    parent_->set_button_enabled(DOWN, state);
    publish_state(state);
  }
 protected:
  BestwaySpa *parent_{nullptr};
};

}  // namespace bestway_spa
}  // namespace esphome
