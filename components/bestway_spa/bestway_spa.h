#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "CIO_TYPE1.h"
#include "enums.h"

namespace esphome {
namespace bestway_spa {

class BestwaySpa : public climate::Climate, public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  void set_protocol_type(ProtocolType type) { protocol_type_ = type; }
  void set_cio_data_pin(InternalGPIOPin *pin) { cio_data_pin_ = pin; }
  void set_cio_clk_pin(InternalGPIOPin *pin) { cio_clk_pin_ = pin; }
  void set_cio_cs_pin(InternalGPIOPin *pin) { cio_cs_pin_ = pin; }

  // Button interaction for Lambdas
  void on_button_press_(ButtonCode code);

  // Climate overrides
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

 protected:
  ProtocolType protocol_type_;
  InternalGPIOPin *cio_data_pin_;
  InternalGPIOPin *cio_clk_pin_;
  InternalGPIOPin *cio_cs_pin_;

  CIO_TYPE1 va_cio_type1;

  void update_sensors();
};

} // namespace bestway_spa
} // namespace esphome
