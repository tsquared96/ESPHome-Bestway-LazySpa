#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "enums.h" // Fixed: Replaced non-existent bestway_va_types.h

namespace esphome {
namespace bestway_spa {

class DSP_TYPE1 {
 public:
  DSP_TYPE1() {}
  void setup(uint8_t data_pin, uint8_t clk_pin, uint8_t cs_pin, uint8_t audio_pin);
  
  // Handlers called by ISR
  void IRAM_ATTR isr_clkHandler();
  
  // State getters
  uint8_t getPressedButton();
  bool has_new_packet() { return _new_packet_available; }
  void clear_packet_flag() { _new_packet_available = false; }

 protected:
  // Uppercase to match the DSP_TYPE1.cpp implementation
  uint8_t _DATA_PIN;
  uint8_t _CLK_PIN;
  uint8_t _CS_PIN;
  uint8_t _AUDIO_PIN;

  uint8_t _byte_count = 0;
  uint8_t _bit_count = 0;
  uint8_t _received_byte = 0;
  bool _packet_transm_active = false;
  bool _new_packet_available = false;

  uint8_t _payload[11] = {0};
  uint8_t _last_btn = 0;
};

}  // namespace bestway_spa
}  // namespace esphome
