#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include <Arduino.h>

namespace esphome {
namespace bestway_spa {

// Button indices - matches VisualApproach
enum Buttons : uint8_t {
  NOBTN = 0,
  LOCK,
  TIMER,
  BUBBLES,
  UNIT,
  HEAT,
  PUMP,
  DOWN,
  UP,
  POWER,
  HYDROJETS,
  BTN_COUNT
};

// CIO states structure
struct CIOStates {
  bool locked = false;
  bool power = true;
  bool unit = true;  // true = Celsius
  bool bubbles = false;
  bool heatgrn = false;
  bool heatred = false;
  bool heat = false;
  bool pump = false;
  bool jets = false;
  bool timerled1 = false;
  bool timerled2 = false;
  bool timerbuttonled = false;
  uint8_t temperature = 20;
  uint8_t target = 38;
  uint8_t error = 0;
  char char1 = ' ';
  char char2 = ' ';
  char char3 = ' ';
};

// Forward declaration
class CIO_TYPE1;

// Global pointer for ISR access
extern CIO_TYPE1 *g_cio_type1_instance;

/**
 * CIO_TYPE1 - Interrupt-driven handler for 6-wire TYPE1 protocol
 * Based on VisualApproach's CIO_TYPE1 implementation
 *
 * This class reads data FROM the CIO board (spa controller) using interrupts.
 * The CIO board drives the clock, and the ESP responds.
 */
class CIO_TYPE1 {
 public:
  CIO_TYPE1();

  void setup(int data_pin, int clk_pin, int cs_pin);
  void stop();
  void pause(bool action);
  void update_states();

  // ISR handlers - must be public for ISR access
  void IRAM_ATTR isr_packet_handler();
  void IRAM_ATTR isr_clk_handler();

  // Accessors
  CIOStates &get_states() { return cio_states_; }
  uint8_t *get_raw_payload() { return raw_payload_from_cio_; }
  bool is_new_packet_available() { return new_packet_available_; }
  uint32_t get_good_packets() { return good_packets_count_; }
  uint32_t get_bad_packets() { return bad_packets_count_; }

  // Button handling
  void set_button_code(uint16_t code) { button_code_ = code; }
  uint16_t get_button_code(Buttons button);
  Buttons get_button_from_code(uint16_t code);

  // Model configuration
  void set_has_jets(bool has_jets) { has_jets_ = has_jets; }
  void set_has_air(bool has_air) { has_air_ = has_air; }
  bool has_jets() { return has_jets_; }
  bool has_air() { return has_air_; }

  // Set button codes for model
  void set_button_codes(const uint16_t *codes, size_t count);

 protected:
  void IRAM_ATTR eop_handler();
  char get_char(uint8_t value);

  // Pin configuration
  int data_pin_{0};
  int clk_pin_{0};
  int cs_pin_{0};

  // Protocol state - volatile for ISR access
  volatile uint8_t byte_count_{0};
  volatile uint8_t bit_count_{0};
  volatile bool data_is_output_{false};
  volatile uint8_t received_byte_{0};
  volatile uint8_t cio_cmd_matches_{0};
  volatile bool new_packet_available_{false};
  volatile uint8_t send_bit_{8};
  volatile uint8_t brightness_{7};
  volatile uint8_t packet_error_{0};
  volatile bool packet_transm_active_{false};
  volatile uint16_t button_code_{0x1B1B};

  // Payload buffers
  volatile uint8_t payload_[11]{0};
  uint8_t raw_payload_from_cio_[11]{0};

  // State
  CIOStates cio_states_;
  uint32_t good_packets_count_{0};
  uint32_t bad_packets_count_{0};

  // Model configuration
  bool has_jets_{false};
  bool has_air_{true};
  uint16_t button_codes_[BTN_COUNT]{0x1B1B, 0x0200, 0x0100, 0x0300, 0x1012, 0x1212, 0x1112, 0x1312, 0x0809, 0x0000, 0x0000};

  // 7-segment character codes (TYPE1)
  static const uint8_t CHARCODES[];
  static const char CHARS[];

  // Command constants
  static const uint8_t DSP_CMD1_MODE6_11_7 = 0x01;
  static const uint8_t DSP_CMD1_MODE6_11_7_P05504 = 0x05;
  static const uint8_t DSP_CMD2_DATAREAD = 0x42;
  static const uint8_t DSP_CMD2_DATAWRITE = 0x40;

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
