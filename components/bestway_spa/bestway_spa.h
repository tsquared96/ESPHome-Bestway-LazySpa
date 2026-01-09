#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "CIO_TYPE1.h"
#include "DSP_TYPE1.h"
#include "enums.h"

namespace esphome {
namespace bestway_spa {

class BestwaySpa : public climate::Climate, public Component {
 public:
  BestwaySpa() {}
  void setup() override;
  void loop() override;
  void dump_config() override;

  // Configuration setters for Python
  void set_protocol_type(ProtocolType type) { protocol_type_ = type; }
  void set_cio_data_pin(InternalGPIOPin *pin) { cio_data_pin_ = pin; }
  void set_cio_clk_pin(InternalGPIOPin *pin) { cio_clk_pin_ = pin; }
  void set_cio_cs_pin(InternalGPIOPin *pin) { cio_cs_pin_ = pin; }
  
  void set_dsp_data_pin(InternalGPIOPin *pin) { dsp_data_pin_ = pin; }
  void set_dsp_clk_pin(InternalGPIOPin *pin) { dsp_clk_pin_ = pin; }
  void set_dsp_cs_pin(InternalGPIOPin *pin) { dsp_cs_pin_ = pin; }

  // Sensor setters used by sensor.py
  void set_current_temperature_sensor(sensor::Sensor *sens) { current_temp_sensor_ = sens; }
  void set_target_temperature_sensor(sensor::Sensor *sens) { target_temp_sensor_ = sens; }

  // Binary sensor setters used by binary_sensor.py
  void set_heating_sensor(binary_sensor::BinarySensor *sens) { heating_sensor_ = sens; }
  void set_filter_sensor(binary_sensor::BinarySensor *sens) { filter_sensor_ = sens; }
  void set_bubbles_sensor(binary_sensor::BinarySensor *sens) { bubbles_sensor_ = sens; }

  // Text sensor setters used by text_sensor.py
  void set_display_text_sensor(text_sensor::TextSensor *sens) { display_text_sensor_ = sens; }

  // Logic bridge
  void on_button_press_(ButtonCode code);

  // Climate implementation
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

 protected:
  ProtocolType protocol_type_;
  InternalGPIOPin *cio_data_pin_;
  InternalGPIOPin *cio_clk_pin_;
  InternalGPIOPin *cio_cs_pin_;
  
  InternalGPIOPin *dsp_data_pin_{nullptr};
  InternalGPIOPin *dsp_clk_pin_{nullptr};
  InternalGPIOPin *dsp_cs_pin_{nullptr};

  sensor::Sensor *current_temp_sensor_{nullptr};
  sensor::Sensor *target_temp_sensor_{nullptr};

  binary_sensor::BinarySensor *heating_sensor_{nullptr};
  binary_sensor::BinarySensor *filter_sensor_{nullptr};
  binary_sensor::BinarySensor *bubbles_sensor_{nullptr};

  text_sensor::TextSensor *display_text_sensor_{nullptr};

  CIO_TYPE1 va_cio_type1;
  DSP_TYPE1 va_dsp_type1;

  void update_sensors();
};

} // namespace bestway_spa
} // namespace esphome
