/**
 * DSP TYPE1 Protocol Handler - Ported from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 *
 * For models: PRE2021, P05504
 *
 * Based on: https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 * Original file: Code/lib/dsp/DSP_TYPE1.cpp
 * Original author: visualapproach
 *
 * This handles communication with the PHYSICAL DISPLAY PANEL:
 * - Sends display data TO the physical display (forwarded from CIO)
 * - Reads button presses FROM the physical display
 *
 * Unlike CIO (interrupt-driven), DSP uses polled bit-banging in main loop.
 */

#include "dsp_type1.h"
#include "esphome/core/log.h"
#include <Arduino.h>

namespace esphome {
namespace bestway_spa {

static const char *const TAG = "dsp_type1";

// Timing constants (from VA firmware)
static const uint32_t DSP_REFRESH_INTERVAL_MS = 50;    // 20Hz display refresh
static const uint32_t DSP_BUTTON_POLL_INTERVAL_MS = 90;  // Button polling interval
static const uint32_t DSP_BIT_DELAY_US = 20;           // Bit timing delay

// Button codes for PRE2021 (TYPE1) - from physical display
static const uint16_t BTN_CODE_NOBTN_T1 = 0xFFFF;
static const uint16_t BTN_CODE_LOCK_T1 = 0x0000;
static const uint16_t BTN_CODE_TIMER_T1 = 0x0001;
static const uint16_t BTN_CODE_BUBBLES_T1 = 0x0002;
static const uint16_t BTN_CODE_UNIT_T1 = 0x0004;
static const uint16_t BTN_CODE_HEAT_T1 = 0x0008;
static const uint16_t BTN_CODE_PUMP_T1 = 0x0010;
static const uint16_t BTN_CODE_DOWN_T1 = 0x0020;
static const uint16_t BTN_CODE_UP_T1 = 0x0080;
static const uint16_t BTN_CODE_POWER_T1 = 0x8000;

// Button codes for P05504 variant
static const uint16_t BTN_CODE_NOBTN_P05504 = 0xFFFF;
static const uint16_t BTN_CODE_LOCK_P05504 = 0x0100;
static const uint16_t BTN_CODE_TIMER_P05504 = 0x0200;
static const uint16_t BTN_CODE_BUBBLES_P05504 = 0x0400;
static const uint16_t BTN_CODE_UNIT_P05504 = 0x0800;
static const uint16_t BTN_CODE_HEAT_P05504 = 0x1000;
static const uint16_t BTN_CODE_PUMP_P05504 = 0x2000;
static const uint16_t BTN_CODE_DOWN_P05504 = 0x4000;
static const uint16_t BTN_CODE_UP_P05504 = 0x8000;

// =============================================================================
// Constructor
// =============================================================================

DspType1::DspType1()
    : _data_pin(-1),
      _clk_pin(-1),
      _cs_pin(-1),
      _audio_pin(-1),
      _enabled(false),
      _last_button(NOBTN),
      _last_button_code(0xFFFF),
      _last_button_time(0),
      _last_refresh_time(0),
      _audio_frequency(0),
      _good_packets(0),
      _bad_packets(0),
      _button_codes(nullptr) {
  clearPayload_();
}

// =============================================================================
// Setup
// =============================================================================

void DspType1::setup(int data_pin, int clk_pin, int cs_pin, int audio_pin) {
  _data_pin = data_pin;
  _clk_pin = clk_pin;
  _cs_pin = cs_pin;
  _audio_pin = audio_pin;

  if (_data_pin < 0 || _clk_pin < 0 || _cs_pin < 0) {
    ESP_LOGW(TAG, "DSP pins not fully configured, DSP disabled");
    _enabled = false;
    return;
  }

  // Configure pins
  pinMode(_data_pin, OUTPUT);
  pinMode(_clk_pin, OUTPUT);
  pinMode(_cs_pin, OUTPUT);

  // Set initial states
  digitalWrite(_data_pin, HIGH);
  digitalWrite(_clk_pin, LOW);
  digitalWrite(_cs_pin, HIGH);

  // Audio pin (optional)
  if (_audio_pin >= 0) {
    pinMode(_audio_pin, OUTPUT);
    digitalWrite(_audio_pin, LOW);
  }

  _enabled = true;
  ESP_LOGI(TAG, "DSP TYPE1 initialized: DATA=%d CLK=%d CS=%d", _data_pin, _clk_pin, _cs_pin);
}

// =============================================================================
// Stop
// =============================================================================

void DspType1::stop() {
  _enabled = false;
  if (_data_pin >= 0) {
    pinMode(_data_pin, INPUT);
  }
  if (_clk_pin >= 0) {
    pinMode(_clk_pin, INPUT);
  }
  if (_cs_pin >= 0) {
    pinMode(_cs_pin, INPUT);
  }
}

// =============================================================================
// Clear Payload
// =============================================================================

void DspType1::clearPayload_() {
  for (int i = 0; i < 11; i++) {
    _payload[i] = 0;
  }
}

// =============================================================================
// Send Bits - Bit-bang data to display (LSB first)
// Based on VA: DSP_TYPE1::sendBits()
// =============================================================================

void DspType1::sendBits_(uint16_t data, int bits) {
  for (int i = 0; i < bits; i++) {
    // Set data bit (LSB first)
    digitalWrite(_data_pin, (data >> i) & 1);
    delayMicroseconds(DSP_BIT_DELAY_US);

    // Clock high
    digitalWrite(_clk_pin, HIGH);
    delayMicroseconds(DSP_BIT_DELAY_US);

    // Clock low
    digitalWrite(_clk_pin, LOW);
    delayMicroseconds(DSP_BIT_DELAY_US);
  }
}

// =============================================================================
// Receive Bits - Read 16-bit button code from display
// Based on VA: DSP_TYPE1::receiveBits()
// Reads bits 8-15 first, then 0-7 (matching VA protocol)
// =============================================================================

uint16_t DspType1::receiveBits_() {
  uint16_t result = 0;

  // Switch data pin to input
  pinMode(_data_pin, INPUT_PULLUP);
  delayMicroseconds(DSP_BIT_DELAY_US);

  // Read 16 bits (bits 8-15 first, then 0-7 per VA protocol)
  for (int i = 0; i < 16; i++) {
    // Clock high
    digitalWrite(_clk_pin, HIGH);
    delayMicroseconds(DSP_BIT_DELAY_US);

    // Read data bit
    bool bit = digitalRead(_data_pin);

    // Store bit (bits 8-15, then 0-7 ordering)
    if (i < 8) {
      result |= (bit ? 1 : 0) << (i + 8);  // Bits 8-15
    } else {
      result |= (bit ? 1 : 0) << (i - 8);  // Bits 0-7
    }

    // Clock low
    digitalWrite(_clk_pin, LOW);
    delayMicroseconds(DSP_BIT_DELAY_US);
  }

  // Switch data pin back to output
  pinMode(_data_pin, OUTPUT);
  digitalWrite(_data_pin, HIGH);

  return result;
}

// =============================================================================
// Character to 7-Segment
// =============================================================================

uint8_t DspType1::charTo7Seg_(char c) {
  // Search in character table
  for (size_t i = 0; i < sizeof(CHARS_DSP); i++) {
    if (CHARS_DSP[i] == c) {
      if (i < sizeof(CHARCODES_TYPE1_DSP)) {
        return CHARCODES_TYPE1_DSP[i];
      }
      break;
    }
  }

  // Default: blank display
  return 0x00;
}

// =============================================================================
// Upload Payload to Display
// Based on VA: DSP_TYPE1::uploadPayload()
// =============================================================================

void DspType1::uploadPayload_(uint8_t brightness) {
  if (!_enabled || _cs_pin < 0 || _clk_pin < 0 || _data_pin < 0) {
    return;
  }

  // Calculate brightness command (0-8 levels)
  uint8_t enableLED = 0;
  if (brightness > 0) {
    enableLED = DSP_DIM_ON;
    brightness -= 1;
  }
  uint8_t dim_cmd = DSP_DIM_BASE | enableLED | (brightness & 0x07);

  // VA sends 4 SEPARATE packets with CS toggling between each:

  // Packet 1: Command 1 (mode)
  delayMicroseconds(30);
  digitalWrite(_cs_pin, LOW);
  sendBits_(DSP_CMD1_MODE6_11_7_P05504, 8);  // Use P05504 variant per VA
  digitalWrite(_cs_pin, HIGH);

  // Packet 2: Command 2 (data write)
  delayMicroseconds(50);
  digitalWrite(_cs_pin, LOW);
  sendBits_(DSP_CMD2_DATAWRITE, 8);
  digitalWrite(_cs_pin, HIGH);

  // Packet 3: 11 payload bytes
  delayMicroseconds(50);
  digitalWrite(_cs_pin, LOW);
  for (int i = 0; i < 11; i++) {
    sendBits_(_payload[i], 8);
  }
  digitalWrite(_cs_pin, HIGH);

  // Packet 4: Brightness/dimming
  delayMicroseconds(50);
  digitalWrite(_cs_pin, LOW);
  sendBits_(dim_cmd, 8);
  digitalWrite(_cs_pin, HIGH);
  delayMicroseconds(50);
}

// =============================================================================
// Handle States - Update display with current spa state
// Based on VA: DSP_TYPE1::handleStates()
// Call at ~20Hz from main loop
// =============================================================================

void DspType1::handleStates(const SpaState& states, uint8_t brightness) {
  if (!_enabled) {
    return;
  }

  uint32_t now = millis();
  if ((now - _last_refresh_time) < DSP_REFRESH_INTERVAL_MS) {
    return;
  }
  _last_refresh_time = now;

  // Clear payload
  clearPayload_();

  // Set display digits (bytes 1, 3, 5)
  _payload[DGT1_IDX] = charTo7Seg_(states.display_chars[0]);
  _payload[DGT2_IDX] = charTo7Seg_(states.display_chars[1]);
  _payload[DGT3_IDX] = charTo7Seg_(states.display_chars[2]);

  // Set command/filler bytes (bytes 0, 2, 4, 6) - per VA protocol these are 0x00
  _payload[0] = 0x00;
  _payload[2] = 0x00;
  _payload[4] = 0x00;
  _payload[6] = 0x00;

  // Set status byte 7 - VA bit positions:
  // TMR2=bit1, TMR1=bit2, LCK=bit3, TMRBTNLED=bit4, REDHTR=bit5, GRNHTR=bit6, AIR=bit7
  uint8_t status1 = 0;
  if (states.timer_active) {
    status1 |= (1 << 1);  // TMR2_BIT
    status1 |= (1 << 2);  // TMR1_BIT
    status1 |= (1 << 4);  // TMRBTNLED_BIT
  }
  if (states.locked) {
    status1 |= (1 << 3);  // LCK_BIT
  }
  if (states.heater_red) {
    status1 |= (1 << 5);  // REDHTR_BIT
  }
  if (states.heater_green) {
    status1 |= (1 << 6);  // GRNHTR_BIT
  }
  if (states.bubbles) {
    status1 |= (1 << 7);  // AIR_BIT
  }
  _payload[7] = status1;

  // Filler byte 8 - per VA protocol this is 0x00
  _payload[8] = 0x00;

  // Set status byte 9 - VA bit positions:
  // FLT=bit1, C=bit2, F=bit3, PWR=bit4, HJT=bit5
  uint8_t status2 = 0;
  if (states.filter_pump) {
    status2 |= (1 << 1);  // FLT_BIT
  }
  if (states.unit_celsius) {
    status2 |= (1 << 2);  // C_BIT
  } else {
    status2 |= (1 << 3);  // F_BIT
  }
  if (states.power) {
    status2 |= (1 << 4);  // PWR_BIT
  }
  if (states.jets) {
    status2 |= (1 << 5);  // HJT_BIT
  }
  _payload[9] = status2;

  // Filler byte 10 - per VA protocol this is 0x00
  _payload[10] = 0x00;

  // Upload to display
  uploadPayload_(brightness);
  _good_packets++;
}

// =============================================================================
// Button Code to Button Index
// Based on VA: DSP_TYPE1::buttonCodeToIndex()
// =============================================================================

Buttons DspType1::buttonCodeToIndex_(uint16_t code) {
  // Check PRE2021 button codes first
  switch (code) {
    case BTN_CODE_NOBTN_T1:
      return NOBTN;
    case BTN_CODE_LOCK_T1:
      return LOCK;
    case BTN_CODE_TIMER_T1:
      return TIMER;
    case BTN_CODE_BUBBLES_T1:
      return BUBBLES;
    case BTN_CODE_UNIT_T1:
      return UNIT;
    case BTN_CODE_HEAT_T1:
      return HEAT;
    case BTN_CODE_PUMP_T1:
      return PUMP;
    case BTN_CODE_DOWN_T1:
      return DOWN;
    case BTN_CODE_UP_T1:
      return UP;
    case BTN_CODE_POWER_T1:
      return POWER;
  }

  // Check P05504 button codes
  switch (code) {
    case BTN_CODE_NOBTN_P05504:
      return NOBTN;
    case BTN_CODE_LOCK_P05504:
      return LOCK;
    case BTN_CODE_TIMER_P05504:
      return TIMER;
    case BTN_CODE_BUBBLES_P05504:
      return BUBBLES;
    case BTN_CODE_UNIT_P05504:
      return UNIT;
    case BTN_CODE_HEAT_P05504:
      return HEAT;
    case BTN_CODE_PUMP_P05504:
      return PUMP;
    case BTN_CODE_DOWN_P05504:
      return DOWN;
    case BTN_CODE_UP_P05504:
      return UP;
  }

  return NOBTN;
}

// =============================================================================
// Update Toggles - Read button press from physical display
// Based on VA: DSP_TYPE1::updateToggles()
// Call at ~90ms intervals from main loop
// =============================================================================

Buttons DspType1::updateToggles() {
  if (!_enabled) {
    return NOBTN;
  }

  uint32_t now = millis();
  if ((now - _last_button_time) < DSP_BUTTON_POLL_INTERVAL_MS) {
    return NOBTN;
  }
  _last_button_time = now;

  // CS low to start button read
  digitalWrite(_cs_pin, LOW);
  delayMicroseconds(DSP_BIT_DELAY_US * 2);

  // Send command byte 1 (mode)
  sendBits_(DSP_CMD1_MODE6_11_7, 8);

  // Send command byte 2 (data read)
  sendBits_(DSP_CMD2_DATAREAD, 8);

  // Read button code
  uint16_t button_code = receiveBits_();

  // CS high to end transmission
  digitalWrite(_cs_pin, HIGH);
  delayMicroseconds(DSP_BIT_DELAY_US * 2);

  // Store raw code
  _last_button_code = button_code;

  // Convert to button enum
  Buttons button = buttonCodeToIndex_(button_code);

  // Only return button if it changed (edge detection)
  if (button != _last_button) {
    Buttons pressed = button;
    _last_button = button;

    if (pressed != NOBTN) {
      ESP_LOGI(TAG, "DSP button pressed: %d (code 0x%04X)", pressed, button_code);
    }
    return pressed;
  }

  return NOBTN;
}

}  // namespace bestway_spa
}  // namespace esphome
