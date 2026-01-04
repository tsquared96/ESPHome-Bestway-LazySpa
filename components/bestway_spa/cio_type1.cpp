/**
 * CIO TYPE1 Protocol Handler - Ported from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 *
 * For models: PRE2021, P05504
 *
 * Based on: https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 * Original file: Code/lib/BWC_6w_type1/CIO_TYPE1.cpp
 * Original author: visualapproach
 *
 * Key protocol details:
 * - 11-byte packets at ~50Hz refresh
 * - LSB first bit order
 * - Rising clock edge: read data from CIO
 * - Falling clock edge: write button data to CIO (when in output mode)
 * - 0x42 command in byte position triggers button read (bits 8-15)
 */

#include "cio_type1.h"

#ifdef ESP8266
#include <eagle_soc.h>  // For READ_PERI_REG, WRITE_PERI_REG
#endif

namespace esphome {
namespace bestway_spa {

// Global instance pointer for ISR access
CioType1* g_cio_type1_instance = nullptr;

// =============================================================================
// Static ISR wrapper functions (call into class instance)
// =============================================================================

static void IRAM_ATTR isr_cs_type1() {
  if (g_cio_type1_instance != nullptr) {
    g_cio_type1_instance->isr_packetHandler();
  }
}

static void IRAM_ATTR isr_clk_type1() {
  if (g_cio_type1_instance != nullptr) {
    g_cio_type1_instance->isr_clkHandler();
  }
}

// =============================================================================
// Constructor
// =============================================================================

CioType1::CioType1()
    : _cio_clk_pin(-1),
      _cio_data_pin(-1),
      _cio_cs_pin(-1),
      _dsp_data_pin(-1),
      _new_packet(false),
      _bit_count(0),
      _byte_count(0),
      _current_byte(0),
      _button_code(0x1B1B),  // NOBTN for PRE2021
      _data_is_output(false),
      _send_bit(0),
      _packet_active(false),
      _good_packets(0),
      _bad_packets(0),
      _clk_count(0),
      _cs_count(0) {
  // Initialize packet buffers
  for (int i = 0; i < 11; i++) {
    _packet[i] = 0;
    _packet_ready[i] = 0;
  }
}

// =============================================================================
// Setup - Initialize pins and attach interrupts
// =============================================================================

void CioType1::setup(int cio_clk_pin, int cio_data_pin, int cio_cs_pin, int dsp_data_pin) {
  _cio_clk_pin = cio_clk_pin;
  _cio_data_pin = cio_data_pin;
  _cio_cs_pin = cio_cs_pin;
  _dsp_data_pin = dsp_data_pin;

  // Set global instance for ISR access
  g_cio_type1_instance = this;

  // --- CIO BUS PINS (input from pump controller) ---

  // CIO Clock - INPUT
  if (_cio_clk_pin >= 0) {
    pinMode(_cio_clk_pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(_cio_clk_pin), isr_clk_type1, CHANGE);
  }

  // CIO Data - INPUT (read display data from pump controller)
  if (_cio_data_pin >= 0) {
    pinMode(_cio_data_pin, INPUT);
  }

  // CIO Chip Select - INPUT with interrupt
  if (_cio_cs_pin >= 0) {
    pinMode(_cio_cs_pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(_cio_cs_pin), isr_cs_type1, CHANGE);
  }

  // --- DSP BUS PIN (output to pump controller) ---

  // DSP Data - OUTPUT for button codes
  if (_dsp_data_pin >= 0) {
    pinMode(_dsp_data_pin, OUTPUT);
    digitalWrite(_dsp_data_pin, HIGH);  // Default high (NOBTN has high bits)
  }

  // Initialize button code to NOBTN
  _button_code = 0x1B1B;
}

// =============================================================================
// Stop - Detach interrupts
// =============================================================================

void CioType1::stop() {
  if (_cio_clk_pin >= 0) {
    detachInterrupt(digitalPinToInterrupt(_cio_clk_pin));
  }
  if (_cio_cs_pin >= 0) {
    detachInterrupt(digitalPinToInterrupt(_cio_cs_pin));
  }
  g_cio_type1_instance = nullptr;
}

// =============================================================================
// Set button code (called from main loop)
// =============================================================================

void CioType1::setButtonCode(uint16_t code) {
  _button_code = code;
}

// =============================================================================
// Check if packet is ready
// =============================================================================

bool CioType1::isPacketReady() {
  return _new_packet;
}

// =============================================================================
// Get packet data (copies to buffer, clears ready flag)
// =============================================================================

bool CioType1::getPacket(uint8_t* buffer, size_t max_len) {
  if (!_new_packet) {
    return false;
  }

  // Copy packet data (disable interrupts briefly)
  noInterrupts();
  size_t len = (max_len < 11) ? max_len : 11;
  for (size_t i = 0; i < len; i++) {
    buffer[i] = _packet_ready[i];
  }
  _new_packet = false;
  interrupts();

  return true;
}

// =============================================================================
// CS (Chip Select) Change ISR - Packet start/end
// Based on VisualApproach isr_packetHandler / eopHandler
// =============================================================================

void IRAM_ATTR CioType1::isr_packetHandler() {
  _cs_count++;

#ifdef ESP8266
  // Fast GPIO read using register access
  bool cs_high = (READ_PERI_REG(PIN_IN) >> _cio_cs_pin) & 1;
#else
  bool cs_high = digitalRead(_cio_cs_pin);
#endif

  if (cs_high) {
    // --- CS went HIGH: End of packet ---
    _packet_active = false;
    _data_is_output = false;

    // Validate packet length (should be 11 bytes = 88 bits)
    // Accept if we got at least 10 complete bytes
    if (_byte_count >= 11 || (_byte_count == 10 && _bit_count > 0)) {
      // Copy to ready buffer
      for (int i = 0; i < 11; i++) {
        _packet_ready[i] = _packet[i];
      }
      _new_packet = true;
      _good_packets++;
    } else if (_byte_count > 0) {
      _bad_packets++;
    }
  } else {
    // --- CS went LOW: Start of new packet ---
    _packet_active = true;
    _bit_count = 0;
    _byte_count = 0;
    _current_byte = 0;
    _data_is_output = false;
    _send_bit = 8;

    // Clear packet buffer
    for (int i = 0; i < 11; i++) {
      _packet[i] = 0;
    }
  }
}

// =============================================================================
// CLK Change ISR - Read/Write data bits
// Based on VisualApproach isr_clkHandler
//
// Rising edge: Read data bit from CIO_DATA (LSB first)
// Falling edge: Write button bit to DSP_DATA (when in output mode)
// =============================================================================

void IRAM_ATTR CioType1::isr_clkHandler() {
  if (!_packet_active) {
    return;
  }

  _clk_count++;

#ifdef ESP8266
  // Fast GPIO read using register access
  bool clk_high = (READ_PERI_REG(PIN_IN) >> _cio_clk_pin) & 1;
  bool data_bit = (READ_PERI_REG(PIN_IN) >> _cio_data_pin) & 1;
#else
  bool clk_high = digitalRead(_cio_clk_pin);
  bool data_bit = digitalRead(_cio_data_pin);
#endif

  if (clk_high) {
    // ===================
    // RISING EDGE: Read data from CIO
    // ===================

    // Shift in bit (LSB first: >> 1 | bit << 7)
    _current_byte = (_current_byte >> 1) | (data_bit << 7);
    _bit_count++;

    if (_bit_count == 8) {
      // Complete byte received
      _bit_count = 0;

      // Store the byte
      if (_byte_count < 11) {
        _packet[_byte_count] = _current_byte;
        _byte_count++;
      }

      // Check if CIO is requesting button data
      // When we receive 0x42 (DSP_CMD2_DATAREAD), we start sending button code
      if (_current_byte == DSP_CMD2_DATAREAD_TYPE1) {
        _data_is_output = true;
        _send_bit = 0;  // Start at bit 0, send all 16 bits
      }

      _current_byte = 0;
    }

  } else {
    // ===================
    // FALLING EDGE: Write button data to DSP_DATA
    // ===================

    if (_data_is_output && _dsp_data_pin >= 0) {
      // Send button code bit (LSB first, all 16 bits)
      bool bit_val = (_button_code >> _send_bit) & 1;

#ifdef ESP8266
      // Fast GPIO write using register access
      if (bit_val) {
        WRITE_PERI_REG(PIN_OUT_SET, 1 << _dsp_data_pin);
      } else {
        WRITE_PERI_REG(PIN_OUT_CLEAR, 1 << _dsp_data_pin);
      }
#else
      digitalWrite(_dsp_data_pin, bit_val ? HIGH : LOW);
#endif

      _send_bit++;

      // Button code is 16 bits total
      if (_send_bit >= 16) {
        _send_bit = 0;
        _data_is_output = false;  // Done sending button code
      }
    }
  }
}

// =============================================================================
// Decode 7-segment character
// =============================================================================

char CioType1::decodeChar(uint8_t segments) {
  // Search for matching segment pattern
  for (size_t i = 0; i < sizeof(CHARCODES_TYPE1_DISP); i++) {
    if (CHARCODES_TYPE1_DISP[i] == segments) {
      return CHARS_TYPE1[i];
    }
  }

  // Handle special cases
  if (segments == 0x00) {
    return ' ';  // Blank display
  }

  return '?';  // Unknown pattern
}

// =============================================================================
// Parse packet into state values
// =============================================================================

void CioType1::parsePacket(const uint8_t* packet,
                           float* current_temp,
                           bool* heater_red, bool* heater_green,
                           bool* pump, bool* bubbles, bool* jets,
                           bool* locked, bool* power, bool* unit_celsius,
                           char* display_chars) {
  // Decode display digits (bytes 1, 3, 5)
  char c1 = decodeChar(packet[1]);
  char c2 = decodeChar(packet[3]);
  char c3 = decodeChar(packet[5]);

  if (display_chars != nullptr) {
    display_chars[0] = c1;
    display_chars[1] = c2;
    display_chars[2] = c3;
    display_chars[3] = '\0';
  }

  // Parse temperature from display
  if (current_temp != nullptr) {
    if (c1 >= '0' && c1 <= '9') {
      // Two or three digit temperature
      int temp = (c1 - '0') * 10;
      if (c2 >= '0' && c2 <= '9') {
        temp += (c2 - '0');
      }
      *current_temp = (float)temp;
    } else if (c2 >= '0' && c2 <= '9' && c3 >= '0' && c3 <= '9') {
      // Two digit temperature (preceded by space or error char)
      *current_temp = (float)((c2 - '0') * 10 + (c3 - '0'));
    }
  }

  // Parse status byte 7 (timer, lock, unit)
  uint8_t status1 = packet[7];
  if (locked != nullptr) {
    *locked = (status1 & LED_LOCK_TYPE1) != 0;
  }
  if (unit_celsius != nullptr) {
    *unit_celsius = (status1 & LED_UNIT_F_TYPE1) == 0;  // 0 = Celsius
  }

  // Parse status byte 9 (heat, air, filter, power, jets)
  uint8_t status2 = packet[9];
  if (heater_green != nullptr) {
    *heater_green = (status2 & LED_HEATGRN_TYPE1) != 0;
  }
  if (bubbles != nullptr) {
    *bubbles = (status2 & LED_BUBBLES_TYPE1) != 0;
  }
  if (pump != nullptr) {
    *pump = (status2 & LED_PUMP_TYPE1) != 0;
  }
  if (heater_red != nullptr) {
    *heater_red = (status2 & LED_HEATRED_TYPE1) != 0;
  }
  if (power != nullptr) {
    *power = (status2 & LED_POWER_TYPE1) != 0;
  }
  if (jets != nullptr) {
    *jets = (status2 & LED_JETS_TYPE1) != 0;
  }
}

}  // namespace bestway_spa
}  // namespace esphome
