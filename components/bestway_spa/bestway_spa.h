#pragma once

#include <queue>
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

// Include the VisualApproach Drivers
#include "CIO_TYPE1.h"
#include "DSP_TYPE1.h"

namespace esphome {
namespace bestway_spa {

// Bridge the namespaces so the compiler knows these types
using Buttons = bestway_va::Buttons;

enum ProtocolType {
  PROTOCOL_4WIRE,
  PROTOCOL_6WIRE_T1,
  PROTOCOL_6WIRE_T2
};

struct SpaState {
  float current_temp{0};
  float target_temp{0};
  bool power{false};
  bool heater_enabled{false};
  bool filter_pump{false};
  bool bubbles{false};
  bool locked{false};
  uint8_t brightness{7};
};

class BestwaySpa : public climate::Climate, public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  // Configuration setters for Pins
  void set_protocol_type(ProtocolType type) { protocol_type_ = type; }
  void set_cio_data_pin(InternalGPIOPin *pin) { cio_data_pin_ = pin; }
  void set_cio_clk_pin(InternalGPIOPin *pin) { cio_clk_pin_ = pin; }
  void set_cio_cs_pin(InternalGPIOPin *pin) { cio_cs_pin_ = pin; }
  void set_dsp_data_pin(InternalGPIOPin *pin) { dsp_data_pin_ = pin; }
  void set_dsp_clk_pin(InternalGPIOPin *pin) { dsp_clk_pin_ = pin; }
  void set_dsp_cs_pin(InternalGPIOPin *pin) { dsp_cs_pin_ = pin; }
  void set_audio_pin(InternalGPIOPin *pin) { audio_pin_ = pin; }

  // Configuration setters for Sensors (Required by climate.py)
  void set_current_temperature_sensor(sensor::Sensor *s) { current_temp_sensor_ = s; }
  void set_target_temperature_sensor(sensor::Sensor *s) { target_temp_sensor_ = s; }
  void set_power_sensor(binary_sensor::BinarySensor *s) { power_sensor_ = s; }
  void set_heating_sensor(binary_sensor::BinarySensor *s) { heating_sensor_ = s; }
  void set_filter_sensor(binary_sensor::BinarySensor *s) { filter_sensor_ = s; }
  void set_bubbles_sensor(binary_sensor::BinarySensor *s) { bubbles_sensor_ = s; }
  void set_display_text_sensor(text_sensor::TextSensor *s) { display_text_sensor_ = s; }

  // Public method for the lambda buttons in your YAML
  void on_button_press_(Buttons btn);

 protected:
  void handle_6wire_protocol_();
  void handle_4wire_protocol_() {} 
  void process_button_queue_();

  // VA Drivers
  bestway_va::CIO_PRE2021 va_cio_type1;
  bestway_va::DSP_TYPE1 va_dsp_type1; 

  // Pin Pointers
  ProtocolType protocol_type_{PROTOCOL_6WIRE_T1};
  InternalGPIOPin *cio_clk_pin_{nullptr};
  InternalGPIOPin *cio_data_pin_{nullptr};
  InternalGPIOPin *cio_cs_pin_{nullptr};
  InternalGPIOPin *dsp_clk_pin_{nullptr};
  InternalGPIOPin *dsp_data_pin_{nullptr};
  InternalGPIOPin *dsp_cs_pin_{nullptr};
  InternalGPIOPin *audio_pin_{nullptr};

  // Sensor Pointers
  sensor::Sensor *current_temp_sensor_{nullptr};
  sensor::Sensor *target_temp_sensor_{nullptr};
  binary_sensor::BinarySensor *power_sensor_{nullptr};
  binary_sensor::BinarySensor *heating_sensor_{nullptr};
  binary_sensor::BinarySensor *filter_sensor_{nullptr};
  binary_sensor::BinarySensor *bubbles_sensor_{nullptr};
  text_sensor::TextSensor *display_text_sensor_{nullptr};

  // Internal State
  SpaState state_;
  bool dsp_enabled_{false};
  std::queue<Buttons> button_queue_;
  uint16_t current_button_code_{0x1B1B};
};

} // namespace bestway_spa
} // namespace esphome
