#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include <vector>

namespace esphome {
namespace bestway_spa {

// Protocol types
enum ProtocolType {
  PROTOCOL_4WIRE,  // 2021+ models
  PROTOCOL_6WIRE   // Pre-2021 models
};

// Button codes (from Visualapproach firmware)
enum ButtonCode {
  BTN_NONE = 0x1B1B,
  BTN_HEATER = 0x1012,
  BTN_FILTER = 0x1013,
  BTN_BUBBLES = 0x1014,
  BTN_TEMP_UP = 0x1015,
  BTN_TEMP_DOWN = 0x1016,
  BTN_LOCK = 0x1017,
  BTN_UNIT = 0x1018
};

// State flags (from Visualapproach STATE bits)
struct SpaState {
  bool locked = false;
  bool power = true;
  bool heater = false;
  bool filter = false;
  bool bubbles = false;
  bool heating_active = false;  // RED LED
  bool heating_ready = false;   // GRN LED
  bool unit_celsius = false;
  uint8_t error_code = 0;
  
  float current_temp = 20.0;
  float target_temp = 37.0;
  
  uint8_t brightness = 8;
};

class BestwaySpa : public climate::Climate, public uart::UARTDevice, public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  
  // Climate interface
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;
  
  // Configuration
  void set_protocol_type(ProtocolType type) { protocol_type_ = type; }
  
  // Sensors
  void set_current_temperature_sensor(sensor::Sensor *sensor) { current_temp_sensor_ = sensor; }
  void set_target_temperature_sensor(sensor::Sensor *sensor) { target_temp_sensor_ = sensor; }
  
  // Binary sensors
  void set_heating_sensor(binary_sensor::BinarySensor *sensor) { heating_sensor_ = sensor; }
  void set_filter_sensor(binary_sensor::BinarySensor *sensor) { filter_sensor_ = sensor; }
  void set_bubbles_sensor(binary_sensor::BinarySensor *sensor) { bubbles_sensor_ = sensor; }
  void set_locked_sensor(binary_sensor::BinarySensor *sensor) { locked_sensor_ = sensor; }
  void set_error_sensor(binary_sensor::BinarySensor *sensor) { error_sensor_ = sensor; }
  
  // Text sensor
  void set_error_text_sensor(text_sensor::TextSensor *sensor) { error_text_sensor_ = sensor; }
  
  // Control methods (called by switches)
  void set_heater(bool state);
  void set_filter(bool state);
  void set_bubbles(bool state);
  void set_lock(bool state);
  void set_unit(bool celsius);
  void adjust_target_temp(int8_t delta);
  
 protected:
  // Protocol handlers
  void handle_4wire_protocol_();
  void handle_6wire_protocol_();
  
  // Packet parsing
  void parse_cio_to_dsp_packet_(const std::vector<uint8_t> &packet);
  void parse_dsp_to_cio_packet_(const std::vector<uint8_t> &packet);
  
  // Packet sending
  void send_button_press_(ButtonCode button);
  void forward_packet_(const std::vector<uint8_t> &packet);
  
  // State management
  void update_climate_state_();
  void update_sensors_();
  void publish_state_();
  
  // Utilities
  float celsius_to_fahrenheit_(float c) { return c * 9.0f / 5.0f + 32.0f; }
  float fahrenheit_to_celsius_(float f) { return (f - 32.0f) * 5.0f / 9.0f; }
  
  ProtocolType protocol_type_{PROTOCOL_4WIRE};
  SpaState state_;
  
  // Sensors
  sensor::Sensor *current_temp_sensor_{nullptr};
  sensor::Sensor *target_temp_sensor_{nullptr};
  binary_sensor::BinarySensor *heating_sensor_{nullptr};
  binary_sensor::BinarySensor *filter_sensor_{nullptr};
  binary_sensor::BinarySensor *bubbles_sensor_{nullptr};
  binary_sensor::BinarySensor *locked_sensor_{nullptr};
  binary_sensor::BinarySensor *error_sensor_{nullptr};
  text_sensor::TextSensor *error_text_sensor_{nullptr};
  
  // Packet buffers
  std::vector<uint8_t> rx_buffer_;
  std::vector<uint8_t> packet_buffer_;
  
  // Timing
  uint32_t last_packet_time_{0};
  uint32_t last_state_update_{0};
  uint32_t last_sensor_update_{0};
  
  // Command queue
  struct PendingCommand {
    ButtonCode button;
    uint32_t time;
    bool processed;
  };
  std::vector<PendingCommand> command_queue_;
  
  // Protocol state
  bool waiting_for_response_{false};
  uint16_t last_button_code_{BTN_NONE};
};

// Switch implementations
class BestwaySpaHeaterSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void write_state(bool state) override { parent_->set_heater(state); }
 protected:
  BestwaySpa *parent_{nullptr};
};

class BestwaySpaFilterSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void write_state(bool state) override { parent_->set_filter(state); }
 protected:
  BestwaySpa *parent_{nullptr};
};

class BestwaySpaBubblesSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void write_state(bool state) override { parent_->set_bubbles(state); }
 protected:
  BestwaySpa *parent_{nullptr};
};

class BestwaySpaLockSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(BestwaySpa *parent) { parent_ = parent; }
  void write_state(bool state) override { parent_->set_lock(state); }
 protected:
  BestwaySpa *parent_{nullptr};
};

}  // namespace bestway_spa
}  // namespace esphome
