/**
 * CIO TYPE2 Protocol Handler - Ported from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 *
 * For models: 54149E
 *
 * Based on: https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 * Original file: Code/lib/BWC_6w_type2/CIO_TYPE2.cpp
 * Original author: visualapproach
 *
 * Protocol differences from TYPE1:
 * - 5-byte packet structure vs 11-byte
 * - Different command/response format
 * - Button code sent when receiving 0x10 command
 * - MSB first bit order for display digits
 */

#include "cio_type2.h"

#ifdef ESP8266
#include <eagle_soc.h>  // For READ_PERI_REG, WRITE_PERI_REG
#endif

namespace esphome {
namespace bestway_spa {

// Global instance pointer for ISR access
CioType2* g_cio_type2_instance = nullptr;

// =============================================================================
// Static ISR wrapper functions
// =============================================================================

static void IRAM_ATTR isr_cs_type2() {
  if (g_cio_type2_instance != nullptr) {
    g_cio_type2_instance->isr_csHandler();
  }
}

static void IRAM_ATTR isr_clk_type2() {
  if (g_cio_type2_instance != nullptr) {
    g_cio_type2_instance->isr_clkHandler();
  }
}

// =============================================================================
// Constructor
// =============================================================================

CioType2::CioType2()
    : _cio_clk_pin(-1),
      _cio_data_pin(-1),
      _cio_cs_pin(-1),
      _dsp_data_pin(-1),
      _new_packet(false),
      _bit_count(0),
      _byte_count(0),
      _current_byte(0),
      _button_code(0x0000),  // NOBTN for 54149E
      _sending_buttons(false),
      _send_bit(0),
      _packet_active(false),
      _cmd_byte(0),
      _good_packets(0),
      _bad_packets(0),
      _clk_count(0),
      _cs_count(0) {
  for (int i = 0; i < 5; i++) {
    _packet[i] = 0;
    _packet_ready[i] = 0;
  }
}

// =============================================================================
// Setup
// =============================================================================

void CioType2::setup(int cio_clk_pin, int cio_data_pin, int cio_cs_pin, int dsp_data_pin) {
  _cio_clk_pin = cio_clk_pin;
  _cio_data_pin = cio_data_pin;
  _cio_cs_pin = cio_cs_pin;
  _dsp_data_pin = dsp_data_pin;

  g_cio_type2_instance = this;

  // CIO Clock - INPUT with CHANGE interrupt
  if (_cio_clk_pin >= 0) {
    pinMode(_cio_clk_pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(_cio_clk_pin), isr_clk_type2, CHANGE);
  }

  // CIO Data - INPUT
  if (_cio_data_pin >= 0) {
    pinMode(_cio_data_pin, INPUT);
  }

  // CIO CS - INPUT with CHANGE interrupt
  if (_cio_cs_pin >= 0) {
    pinMode(_cio_cs_pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(_cio_cs_pin), isr_cs_type2, CHANGE);
  }

  // DSP Data - OUTPUT
  if (_dsp_data_pin >= 0) {
    pinMode(_dsp_data_pin, OUTPUT);
    digitalWrite(_dsp_data_pin, LOW);  // Default low for TYPE2
  }

  _button_code = 0x0000;  // NOBTN for 54149E
}

// =============================================================================
// Stop
// =============================================================================

void CioType2::stop() {
  if (_cio_clk_pin >= 0) {
    detachInterrupt(digitalPinToInterrupt(_cio_clk_pin));
  }
  if (_cio_cs_pin >= 0) {
    detachInterrupt(digitalPinToInterrupt(_cio_cs_pin));
  }
  g_cio_type2_instance = nullptr;
}

// =============================================================================
// Button code setter
// =============================================================================

void CioType2::setButtonCode(uint16_t code) {
  _button_code = code;
}

// =============================================================================
// Packet ready check
// =============================================================================

bool CioType2::isPacketReady() {
  return _new_packet;
}

// =============================================================================
// Get packet
// =============================================================================

bool CioType2::getPacket(uint8_t* buffer, size_t max_len) {
  if (!_new_packet) {
    return false;
  }

  noInterrupts();
  size_t len = (max_len < 5) ? max_len : 5;
  for (size_t i = 0; i < len; i++) {
    buffer[i] = _packet_ready[i];
  }
  _new_packet = false;
  interrupts();

  return true;
}

// =============================================================================
// CS Handler - Packet delimiter
// =============================================================================

void IRAM_ATTR CioType2::isr_csHandler() {
  _cs_count++;

#ifdef ESP8266
  bool cs_high = (READ_PERI_REG(PIN_IN) >> _cio_cs_pin) & 1;
#else
  bool cs_high = digitalRead(_cio_cs_pin);
#endif

  if (cs_high) {
    // CS HIGH - End of packet
    _packet_active = false;
    _sending_buttons = false;

    // Validate and copy packet (5 bytes for TYPE2)
    if (_byte_count >= 5 || (_byte_count == 4 && _bit_count > 0)) {
      for (int i = 0; i < 5; i++) {
        _packet_ready[i] = _packet[i];
      }
      _new_packet = true;
      _good_packets++;
    } else if (_byte_count > 0) {
      _bad_packets++;
    }

  } else {
    // CS LOW - Start of packet
    _packet_active = true;
    _bit_count = 0;
    _byte_count = 0;
    _current_byte = 0;
    _sending_buttons = false;
    _cmd_byte = 0;

    for (int i = 0; i < 5; i++) {
      _packet[i] = 0;
    }
  }
}

// =============================================================================
// CLK Handler - Read/Write bits
// TYPE2 uses different timing than TYPE1
// =============================================================================

void IRAM_ATTR CioType2::isr_clkHandler() {
  if (!_packet_active) {
    return;
  }

  _clk_count++;

#ifdef ESP8266
  bool clk_high = (READ_PERI_REG(PIN_IN) >> _cio_clk_pin) & 1;
  bool data_bit = (READ_PERI_REG(PIN_IN) >> _cio_data_pin) & 1;
#else
  bool clk_high = digitalRead(_cio_clk_pin);
  bool data_bit = digitalRead(_cio_data_pin);
#endif

  if (clk_high) {
    // ===================
    // RISING EDGE: Read data from CIO
    // TYPE2 uses MSB first for display bytes, LSB first for command
    // ===================

    // Shift in bit (LSB first for command byte)
    _current_byte = (_current_byte >> 1) | (data_bit << 7);
    _bit_count++;

    if (_bit_count == 8) {
      _bit_count = 0;

      if (_byte_count < 5) {
        _packet[_byte_count] = _current_byte;

        // First byte is command byte
        if (_byte_count == 0) {
          _cmd_byte = _current_byte;
        }

        _byte_count++;
      }

      // Check if CIO wants button data (command 0x10)
      if (_current_byte == CIO_CMD_GETBUTTONS_TYPE2) {
        _sending_buttons = true;
        _send_bit = 0;
      }

      _current_byte = 0;
    }

  } else {
    // ===================
    // FALLING EDGE: Write button data
    // ===================

    if (_sending_buttons && _dsp_data_pin >= 0) {
      // Send button code bits (LSB first)
      bool bit_val = (_button_code >> _send_bit) & 1;

#ifdef ESP8266
      if (bit_val) {
        WRITE_PERI_REG(PIN_OUT_SET, 1 << _dsp_data_pin);
      } else {
        WRITE_PERI_REG(PIN_OUT_CLEAR, 1 << _dsp_data_pin);
      }
#else
      digitalWrite(_dsp_data_pin, bit_val ? HIGH : LOW);
#endif

      _send_bit++;

      // TYPE2 button code is 16 bits
      if (_send_bit >= 16) {
        _send_bit = 0;
        _sending_buttons = false;
      }
    }
  }
}

// =============================================================================
// Decode 7-segment character
// =============================================================================

char CioType2::decodeChar(uint8_t segments) {
  for (size_t i = 0; i < sizeof(CHARCODES_TYPE2_DISP); i++) {
    if (CHARCODES_TYPE2_DISP[i] == segments) {
      return CHARS_TYPE2[i];
    }
  }

  if (segments == 0x00) {
    return ' ';
  }

  return '?';
}

// =============================================================================
// Parse packet into state values
// TYPE2 packet structure:
// Byte 0: Command
// Byte 1: Digit 1 (7-segment)
// Byte 2: Digit 2 (7-segment)
// Byte 3: Digit 3 (7-segment)
// Byte 4: LED status bits
// =============================================================================

void CioType2::parsePacket(const uint8_t* packet,
                           float* current_temp,
                           bool* heater_red, bool* heater_green,
                           bool* pump, bool* bubbles, bool* jets,
                           bool* locked, bool* power, bool* unit_celsius,
                           char* display_chars) {
  // Decode display digits (bytes 1, 2, 3)
  char c1 = decodeChar(packet[1]);
  char c2 = decodeChar(packet[2]);
  char c3 = decodeChar(packet[3]);

  if (display_chars != nullptr) {
    display_chars[0] = c1;
    display_chars[1] = c2;
    display_chars[2] = c3;
    display_chars[3] = '\0';
  }

  // Parse temperature from display
  if (current_temp != nullptr) {
    if (c1 >= '0' && c1 <= '9') {
      int temp = (c1 - '0') * 10;
      if (c2 >= '0' && c2 <= '9') {
        temp += (c2 - '0');
      }
      *current_temp = (float)temp;
    } else if (c2 >= '0' && c2 <= '9' && c3 >= '0' && c3 <= '9') {
      *current_temp = (float)((c2 - '0') * 10 + (c3 - '0'));
    }
  }

  // Parse LED status byte (byte 4)
  uint8_t leds = packet[4];

  if (power != nullptr) {
    *power = (leds & LED_POWER_TYPE2) != 0;
  }
  if (bubbles != nullptr) {
    *bubbles = (leds & LED_BUBBLES_TYPE2) != 0;
  }
  if (jets != nullptr) {
    *jets = (leds & LED_JETS_TYPE2) != 0;
  }
  if (heater_green != nullptr) {
    *heater_green = (leds & LED_HEATGRN_TYPE2) != 0;
  }
  if (heater_red != nullptr) {
    *heater_red = (leds & LED_HEATRED_TYPE2) != 0;
  }
  if (pump != nullptr) {
    *pump = (leds & LED_PUMP_TYPE2) != 0;
  }
  if (unit_celsius != nullptr) {
    *unit_celsius = (leds & LED_UNIT_F_TYPE2) == 0;  // 0 = Celsius
  }
  if (locked != nullptr) {
    *locked = (leds & LED_LOCK_TYPE2) != 0;
  }
}

}  // namespace bestway_spa
}  // namespace esphome
