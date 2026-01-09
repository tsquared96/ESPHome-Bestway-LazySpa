#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "CIO_TYPE1.h"
#include "DSP_TYPE1.h"
#include "enums.h"

namespace esphome {
namespace bestway_spa {

class BestwaySpa : public climate::Climate, public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  // Setters for Python/YAML
  void set_protocol_type(ProtocolType type) { protocol_type_ = type; }
  void set_cio_data_pin(InternalGPIOPin *pin) { cio_data_pin_ = pin; }
  void set_cio_clk_pin(InternalGPIOPin *pin) { cio_clk_pin_ = pin; }
  void set_cio_cs_pin(InternalGPIOPin *pin) { cio_cs_pin_ = pin; }
  
  void set_dsp_data_pin(InternalGPIOPin *pin) { dsp_data_pin_ = pin; }
  void set_dsp_clk_pin(InternalGPIOPin *pin) { dsp_clk_pin_ = pin; }
  void set_dsp_cs_pin(InternalGPIOPin *pin) { dsp_cs_pin_ = pin; }

  // FIX: Missing Method Declaration from Claude's review
  void on_button_press_(ButtonCode code);

  // Climate overrides
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  std::string get_display_text_sensor() { return va_cio_type1.get_display(); }

 protected:
  ProtocolType protocol_type_;
  InternalGPIOPin *cio_data_pin_;
  InternalGPIOPin *cio_clk_pin_;
  InternalGPIOPin *cio_cs_pin_;
  
  InternalGPIOPin *dsp_data_pin_{nullptr};
  InternalGPIOPin *dsp_clk_pin_{nullptr};
  InternalGPIOPin *dsp_cs_pin_{nullptr};

  CIO_TYPE1 va_cio_type1;
  DSP_TYPE1 va_dsp_type1;

  void update_sensors();
};

} // namespace bestway_spa
} // namespace esphome
