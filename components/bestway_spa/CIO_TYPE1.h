#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "enums.h"

namespace esphome {
namespace bestway_spa {

class CIO_TYPE1 {
 public:
  CIO_TYPE1() {}
  void setup(uint8_t data_pin, uint8_t clk_pin, uint8_t cs_pin);
  void updateStates();
  void setButtonCode(uint8_t code) { _button_code = code; }
  
  // State getters
  float get_current_temp() { return _current_temp; }
  float get_target_temp() { return _target_temp; }
  bool is_heating() { return _heating; }
  bool is_filtering() { return _filtering; }
  bool is_bubbles() { return _bubbles; }
  const char* get_display() { return _display_text; }

  // Handlers called by ISR
  void IRAM_ATTR isr_clkHandler();
  void IRAM_ATTR isr_packetHandler();
  void IRAM_ATTR eopHandler();

 protected:
  uint8_t _DATA_PIN;
  uint8_t _CLK_PIN;
  uint8_t _CS_PIN;

  uint8_t _button_code = 0;
  uint8_t _byte_count = 0;
  uint8_t _received_byte = 0;
  uint8_t _send_bit = 0;
  uint8_t _CIO_cmd_matches = 0;
  bool _packet_error = false;
  bool _packet_transm_active = false;
  bool _data_is_output = false;
  bool _new_packet_available = false;
  
  uint8_t _payload[11] = {0};
  char _display_text[10] = {0};
  
  float _current_temp = 0;
  float _target_temp = 0;
  bool _heating = false;
  bool _filtering = false;
  bool _bubbles = false;

  uint32_t good_packets_count = 0;
  uint32_t bad_packets_count = 0;

  char _getChar(uint8_t code);
};

} // namespace bestway_spa
} // namespace esphome
