/**
 * DSP TYPE1 Protocol Handler - Ported from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 *
 * For models: PRE2021, P05504
 *
 * Based on: https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 * Original file: Code/lib/dsp/DSP_TYPE1.cpp/h
 * Original author: visualapproach
 *
 * This handles communication with the PHYSICAL DISPLAY PANEL:
 * - Sends display data TO the physical display (forwarded from CIO)
 * - Reads button presses FROM the physical display
 *
 * Unlike CIO (interrupt-driven), DSP uses polled bit-banging in main loop.
 */

#pragma once

#include <Arduino.h>
#include "bestway_spa.h"

namespace esphome {
namespace bestway_spa {

// DSP Commands
static const uint8_t DSP_CMD1_MODE6_11_7 = 0x01;
static const uint8_t DSP_CMD1_MODE6_11_7_P05504 = 0x05;
static const uint8_t DSP_CMD2_DATAWRITE = 0x00;
static const uint8_t DSP_CMD2_DATAREAD = 0x42;

// Brightness
static const uint8_t DSP_DIM_BASE = 0x80;
static const uint8_t DSP_DIM_ON = 0x08;

// 7-segment character codes for display output
static const uint8_t CHARCODES_TYPE1_DSP[] = {
  0x7F, 0x0D, 0xB7, 0x9F, 0xCD, 0xDB, 0xFB, 0x0F, 0xFF, 0xDF,  // 0-9
  0xEF, 0xF9, 0x73, 0xBD, 0xF3, 0xE3, 0x7B, 0xE9, 0x09, 0x3D,  // A-J
  0xE1, 0x71, 0x49, 0xA9, 0xB9, 0xE7, 0xCF, 0xA1, 0xDB, 0xF1,  // K-T
  0x7D, 0x7D, 0x7D, 0xED, 0xDD, 0xB7, 0x01, 0x81              // U-Z, space, dash
};

static const char CHARS_DSP[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ -";

// Payload byte indices for TYPE1 display
// Display digits
static const uint8_t DGT1_IDX = 1;
static const uint8_t DGT2_IDX = 3;
static const uint8_t DGT3_IDX = 5;

// Status byte 7 bit positions
static const uint8_t TMR1_IDX = 7;
static const uint8_t TMR1_BIT = 1;
static const uint8_t TMR2_IDX = 7;
static const uint8_t TMR2_BIT = 2;
static const uint8_t LCK_IDX = 7;
static const uint8_t LCK_BIT = 3;
static const uint8_t TMRBTNLED_IDX = 7;
static const uint8_t TMRBTNLED_BIT = 4;
static const uint8_t REDHTR_IDX = 7;
static const uint8_t REDHTR_BIT = 5;
static const uint8_t GRNHTR_IDX = 7;
static const uint8_t GRNHTR_BIT = 6;
static const uint8_t AIR_IDX = 7;
static const uint8_t AIR_BIT = 7;

// Status byte 9 bit positions
static const uint8_t FLT_IDX = 9;
static const uint8_t FLT_BIT = 1;
static const uint8_t C_IDX = 9;
static const uint8_t C_BIT = 2;
static const uint8_t F_IDX = 9;
static const uint8_t F_BIT = 3;
static const uint8_t PWR_IDX = 9;
static const uint8_t PWR_BIT = 4;
static const uint8_t HJT_IDX = 9;
static const uint8_t HJT_BIT = 5;

/**
 * DSP TYPE1 class - handles physical display communication
 *
 * This is polled (not interrupt-driven) and runs in the main loop.
 */
class DspType1 {
 public:
  DspType1();

  /**
   * Initialize DSP communication
   * @param data_pin GPIO for data line (bidirectional)
   * @param clk_pin GPIO for clock line (output)
   * @param cs_pin GPIO for chip select line (output)
   * @param audio_pin GPIO for audio/buzzer (output, optional)
   */
  void setup(int data_pin, int clk_pin, int cs_pin, int audio_pin = -1);

  /**
   * Stop DSP communication
   */
  void stop();

  /**
   * Update display with current state (call from main loop)
   * Sends display data to physical display at ~20Hz
   * @param states Current spa state to display
   * @param brightness Display brightness (0-8)
   */
  void handleStates(const SpaState& states, uint8_t brightness);

  /**
   * Read button press from physical display (call from main loop)
   * Polls at ~90ms intervals
   * @return Button code, or NOBTN if no button pressed
   */
  Buttons updateToggles();

  /**
   * Get last pressed button
   */
  Buttons getLastButton() const { return _last_button; }

  /**
   * Get raw button code from last read
   */
  uint16_t getLastButtonCode() const { return _last_button_code; }

  /**
   * Set audio frequency (0 to disable)
   */
  void setAudioFrequency(uint16_t freq) { _audio_frequency = freq; }

  /**
   * Check if DSP is enabled
   */
  bool isEnabled() const { return _enabled; }

  // Statistics
  uint32_t getGoodPackets() const { return _good_packets; }
  uint32_t getBadPackets() const { return _bad_packets; }

 protected:
  /**
   * Send bits to display (bit-banging)
   * @param data Data to send
   * @param bits Number of bits to send
   */
  void sendBits_(uint16_t data, int bits);

  /**
   * Receive bits from display (bit-banging)
   * @return 16-bit value read from display
   */
  uint16_t receiveBits_();

  /**
   * Upload payload to display
   * @param brightness Display brightness (0-8)
   */
  void uploadPayload_(uint8_t brightness);

  /**
   * Convert character to 7-segment code
   */
  uint8_t charTo7Seg_(char c);

  /**
   * Convert button code to button enum
   */
  Buttons buttonCodeToIndex_(uint16_t code);

  /**
   * Clear payload buffer
   */
  void clearPayload_();

 private:
  // GPIO pins
  int _data_pin{-1};
  int _clk_pin{-1};
  int _cs_pin{-1};
  int _audio_pin{-1};

  // State
  bool _enabled{false};
  uint8_t _payload[11]{0};

  // Button reading
  Buttons _last_button{NOBTN};
  uint16_t _last_button_code{0xFFFF};
  uint32_t _last_button_time{0};

  // Display refresh
  uint32_t _last_refresh_time{0};

  // Audio
  uint16_t _audio_frequency{0};

  // Statistics
  uint32_t _good_packets{0};
  uint32_t _bad_packets{0};

  // Button codes for TYPE1 (same as CIO side)
  const uint16_t* _button_codes{nullptr};
};

}  // namespace bestway_spa
}  // namespace esphome
