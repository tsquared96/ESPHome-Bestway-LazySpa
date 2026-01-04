#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "cio_type1.h"
#include <Arduino.h>

namespace esphome {
namespace bestway_spa {

// DSP states structure
struct DSPStates {
  bool locked = false;
  bool power = true;
  bool unit = true;  // true = Celsius
  bool bubbles = false;
  bool heatgrn = false;
  bool heatred = false;
  bool pump = false;
  bool jets = false;
  bool timerled1 = false;
  bool timerled2 = false;
  bool timerbuttonled = false;
  uint8_t brightness = 7;
  char char1 = ' ';
  char char2 = ' ';
  char char3 = ' ';
};

/**
 * DSP_TYPE1 - Bit-banging handler for 6-wire TYPE1 display
 * Based on VisualApproach's DSP_TYPE1 implementation
 *
 * This class sends data TO the DSP board (display panel) using bit-banging.
 * The ESP drives the clock.
 */
class DSP_TYPE1 {
 public:
  DSP_TYPE1();

  void setup(int data_pin, int clk_pin, int cs_pin, int audio_pin);
  void stop();

  // Update display with current states
  void handle_states();

  // Get button press from DSP
  Buttons get_pressed_button();

  // Accessors
  DSPStates &get_states() { return dsp_states_; }
  uint8_t *get_raw_payload() { return raw_payload_from_dsp_; }
  int get_audio_pin() { return audio_pin_; }

  // Text overlay
  void set_text(const String &text) { text_ = text; }
  void clear_text() { text_ = ""; }

  // Audio control
  void set_audio_frequency(uint16_t freq) { audio_frequency_ = freq; }

 protected:
  void send_bits_to_dsp(uint32_t out_bits, int bits_to_send);
  uint16_t receive_bits_from_dsp();
  void upload_payload(uint8_t brightness);
  void clear_payload();
  uint8_t char_to_7seg_code(char c);
  Buttons button_code_to_index(uint16_t code);

  // Pin configuration
  int data_pin_{0};
  int clk_pin_{0};
  int cs_pin_{0};
  int audio_pin_{0};

  // Timing
  uint32_t last_refresh_time_{0};
  uint32_t last_button_poll_time_{0};

  // Payload buffer (11 bytes)
  uint8_t payload_[11]{0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint8_t raw_payload_from_dsp_[2]{0};

  // State
  DSPStates dsp_states_;
  Buttons old_button_{NOBTN};
  String text_;
  uint16_t audio_frequency_{0};

  // Button codes for DSP (these are different from CIO button codes)
  uint16_t button_codes_[BTN_COUNT]{0x1B1B, 0x0200, 0x0100, 0x0300, 0x1012, 0x1212, 0x1112, 0x1312, 0x0809, 0x0000, 0x0000};

  // 7-segment character codes
  static const uint8_t CHARCODES[];
  static const char CHARS[];

  // Command constants
  static const uint8_t DSP_CMD1_MODE6_11_7 = 0x01;
  static const uint8_t DSP_CMD1_MODE6_11_7_P05504 = 0x05;
  static const uint8_t DSP_CMD2_DATAREAD = 0x42;
  static const uint8_t DSP_CMD2_DATAWRITE = 0x40;
  static const uint8_t DSP_DIM_BASE = 0x80;
  static const uint8_t DSP_DIM_ON = 0x08;

  // Payload indices
  static const uint8_t DGT1_IDX = 1;
  static const uint8_t DGT2_IDX = 3;
  static const uint8_t DGT3_IDX = 5;
  static const uint8_t LCK_IDX = 7;
  static const uint8_t LCK_BIT = 2;
  static const uint8_t TMR1_IDX = 7;
  static const uint8_t TMR1_BIT = 1;
  static const uint8_t TMR2_IDX = 7;
  static const uint8_t TMR2_BIT = 0;
  static const uint8_t TMRBTNLED_IDX = 7;
  static const uint8_t TMRBTNLED_BIT = 3;
  static const uint8_t REDHTR_IDX = 9;
  static const uint8_t REDHTR_BIT = 3;
  static const uint8_t GRNHTR_IDX = 9;
  static const uint8_t GRNHTR_BIT = 0;
  static const uint8_t AIR_IDX = 9;
  static const uint8_t AIR_BIT = 1;
  static const uint8_t FLT_IDX = 9;
  static const uint8_t FLT_BIT = 2;
  static const uint8_t C_IDX = 7;
  static const uint8_t C_BIT = 0;
  static const uint8_t F_IDX = 9;
  static const uint8_t F_BIT = 4;
  static const uint8_t PWR_IDX = 9;
  static const uint8_t PWR_BIT = 5;
  static const uint8_t HJT_IDX = 9;
  static const uint8_t HJT_BIT = 6;
};

}  // namespace bestway_spa
}  // namespace esphome
