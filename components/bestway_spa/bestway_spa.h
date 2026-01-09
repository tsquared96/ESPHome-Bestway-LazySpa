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

  // Configuration setters
  void set_protocol_type(ProtocolType type) { protocol_type_ = type; }
  void set_cio_data_pin(InternalGPIOPin *pin) { cio_data_pin_ = pin; }
  void set_cio_clk_pin(InternalGPIOPin *pin) { cio_clk_pin_ = pin; }
  void set_cio_cs_pin(InternalGPIOPin *pin) { cio_cs_pin_ = pin; }
  void set_dsp_data_pin(InternalGPIOPin *pin) { dsp_data_pin_ = pin; }
  void set_dsp_clk_pin(InternalGPIOPin *pin) { dsp_clk_pin_ = pin; }
  void set_dsp_cs_pin(InternalGPIOPin *pin) { dsp_cs_pin_ = pin; }
  void set_audio_pin(InternalGPIOPin *pin) { audio_pin_ = pin; }

 protected:
  void handle_6wire_protocol_();
  void handle_4wire_protocol_() {} // Placeholder
  void process_button_queue_();
  void on_button_press_(Buttons btn);

  // VA Drivers
  bestway_va::CIO_PRE2021 va_cio_type1;
  bestway_va::DSP_PRE2021 va_dsp_type1;

  // Pins
  ProtocolType protocol_type_{PROTOCOL_6WIRE_T1};
  InternalGPIOPin *cio_clk_pin_{nullptr};
  InternalGPIOPin *cio_data_pin_{nullptr};
  InternalGPIOPin *cio_cs_pin_{nullptr};
  InternalGPIOPin *dsp_clk_pin_{nullptr};
  InternalGPIOPin *dsp_data_pin_{nullptr};
  InternalGPIOPin *dsp_cs_pin_{nullptr};
  InternalGPIOPin *audio_pin_{nullptr};

  // Internal State
  SpaState state_;
  bool dsp_enabled_{false};
  bestway_va::Buttons last_physical_btn_{bestway_va::NOBTN};
  uint32_t last_pkt_count_{0};
  uint32_t last_update_{0};
  uint32_t last_button_time_{0};
  
  std::queue<Buttons> button_queue_;
  uint16_t current_button_code_{0x1B1B};
};

} // namespace bestway_spa
} // namespace esphome
