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

  // Configuration setters for Python/YAML
  void set_protocol_type(ProtocolType type) { protocol_type_ = type; }
  
  // CIO Pin Setters
  void set_cio_data_pin(InternalGPIOPin *pin) { cio_data_pin_ = pin; }
  void set_cio_clk_pin(InternalGPIOPin *pin) { cio_clk_pin_ = pin; }
  void set_cio_cs_pin(InternalGPIOPin *pin) { cio_cs_pin_ = pin; }

  // DSP Pin Setters (Missing until now)
  void set_dsp_data_pin(InternalGPIOPin *pin) { dsp_data_pin_ = pin; }
  void set_dsp_clk_pin(InternalGPIOPin *pin) { dsp_clk_pin_ = pin; }
  void set_dsp_cs_pin(InternalGPIOPin *pin) { dsp_cs_pin_ = pin; }

  // Climate overrides
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  // Helper for text sensors/lambdas
  std::string get_display_text_sensor() { return va_cio_type1.get_display(); }

 protected:
  ProtocolType protocol_type_;
  
  InternalGPIOPin *cio_data_pin_;
  InternalGPIOPin *cio_clk_pin_;
  InternalGPIOPin *cio_cs_pin_;

  // Optional DSP pins
  InternalGPIOPin *dsp_data_pin_{nullptr};
  InternalGPIOPin *dsp_clk_pin_{nullptr};
  InternalGPIOPin *dsp_cs_pin_{nullptr};

  CIO_TYPE1 va_cio_type1;
  DSP_TYPE1 va_dsp_type1; // Added to match the DSP headers

  void update_sensors();
};

} // namespace bestway_spa
} // namespace esphome
