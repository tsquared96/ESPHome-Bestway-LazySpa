#pragma once
/**
 * CIO TYPE1 Protocol Handler - Ported from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 *
 * For models: PRE2021, P05504
 *
 * Protocol: 6-wire SPI-like, 11-byte packets
 * - Clock: Interrupt on CHANGE (both edges)
 * - Rising edge: Read data bits (LSB first)
 * - Falling edge: Write button bits
 * - CS: Packet delimiter (LOW=start, HIGH=end)
 * - DSP_CMD2_DATAREAD (0x42): Triggers button transmission on bits 8-15
 *
 * Based on: https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 * Original author: visualapproach
 */

#include <Arduino.h>
#include <stdint.h>

namespace esphome {
namespace bestway_spa {

// Forward declaration
class BestwaySpa;

// =============================================================================
// CIO TYPE1 Constants - From VisualApproach
// =============================================================================

// CIO to DSP command bytes
static const uint8_t DSP_CMD1_TYPE1 = 0x00;
static const uint8_t DSP_CMD2_DATAREAD_TYPE1 = 0x42;  // Triggers button read

// DSP to CIO status bytes
static const uint8_t CIO_ACK1_TYPE1 = 0x00;
static const uint8_t CIO_ACK2_TYPE1 = 0x00;

// Payload structure for TYPE1 (11 bytes total)
// Byte 0: CMD1 (always 0x00)
// Byte 1: Digit 1 segment pattern
// Byte 2: CMD2 (0x42 = data read request)
// Byte 3: Digit 2 segment pattern
// Byte 4: CMD1 (repeated)
// Byte 5: Digit 3 segment pattern
// Byte 6: (unknown)
// Byte 7: LED/status byte 1 (timer, lock, unit)
// Byte 8: (unknown)
// Byte 9: LED/status byte 2 (heat, air, filter, power, jets)
// Byte 10: (unknown)

// LED bit masks for status byte 7 (TYPE1)
static const uint8_t LED_LOCK_TYPE1    = 0x04;
static const uint8_t LED_TIMER_TYPE1   = 0x02;
static const uint8_t LED_UNIT_F_TYPE1  = 0x01;  // 0=Celsius, 1=Fahrenheit

// LED bit masks for status byte 9 (TYPE1)
static const uint8_t LED_HEATGRN_TYPE1  = 0x01;  // Heat standby (green)
static const uint8_t LED_BUBBLES_TYPE1  = 0x02;  // Air/bubbles
static const uint8_t LED_PUMP_TYPE1     = 0x04;  // Filter pump
static const uint8_t LED_HEATRED_TYPE1  = 0x08;  // Heating active (red)
static const uint8_t LED_POWER_TYPE1    = 0x20;  // Power on
static const uint8_t LED_JETS_TYPE1     = 0x40;  // HydroJets (if available)

// 7-segment character codes for TYPE1 (38 characters: 0-9, A-Z, space, dash)
static const uint8_t CHARCODES_TYPE1_DISP[] = {
  0x7F, 0x0D, 0xB7, 0x9F, 0xCD, 0xDB, 0xFB, 0x0F, 0xFF, 0xDF,  // 0-9
  0xEF, 0xF9, 0x73, 0xBD, 0xF3, 0xE3, 0x7B, 0xE9, 0x09, 0x3D,  // A-J
  0xE1, 0x71, 0x49, 0xA9, 0xB9, 0xE7, 0xCF, 0xA1, 0xDB, 0xF1,  // K-T
  0x7D, 0x7D, 0x7D, 0xED, 0xDD, 0xB7, 0x00, 0x80              // U-Z, space, dash
};

// Character lookup
static const char CHARS_TYPE1[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ -";

// =============================================================================
// CIO TYPE1 Class - Ported from VisualApproach
// =============================================================================

class CioType1 {
 public:
  CioType1();

  // Initialize pins and attach interrupts
  void setup(int cio_clk_pin, int cio_data_pin, int cio_cs_pin, int dsp_data_pin);

  // Detach interrupts
  void stop();

  // Set the button code to transmit (called from main loop)
  void setButtonCode(uint16_t code);

  // Check if new packet is available
  bool isPacketReady();

  // Get the latest packet data (copies to provided buffer, returns true if valid)
  bool getPacket(uint8_t* buffer, size_t max_len);

  // Decode 7-segment character
  static char decodeChar(uint8_t segments);

  // Parse packet into state values
  static void parsePacket(const uint8_t* packet,
                          float* current_temp,
                          bool* heater_red, bool* heater_green,
                          bool* pump, bool* bubbles, bool* jets,
                          bool* locked, bool* power, bool* unit_celsius,
                          char* display_chars);

  // Get statistics
  uint32_t getGoodPackets() const { return _good_packets; }
  uint32_t getBadPackets() const { return _bad_packets; }
  uint32_t getClkCount() const { return _clk_count; }
  uint32_t getCsCount() const { return _cs_count; }

  // ISR handlers (called by static wrapper functions)
  void IRAM_ATTR isr_packetHandler();  // CS change handler
  void IRAM_ATTR isr_clkHandler();     // CLK change handler

 private:
  // Pin numbers (for direct GPIO access in ISR)
  int _cio_clk_pin;
  int _cio_data_pin;
  int _cio_cs_pin;
  int _dsp_data_pin;

  // Packet buffer (11 bytes for TYPE1)
  volatile uint8_t _packet[11];
  volatile uint8_t _packet_ready[11];  // Double buffer for main loop
  volatile bool _new_packet;
  volatile uint8_t _bit_count;
  volatile uint8_t _byte_count;
  volatile uint8_t _current_byte;

  // Button transmission
  volatile uint16_t _button_code;
  volatile bool _data_is_output;
  volatile uint8_t _send_bit;

  // Protocol state
  volatile bool _packet_active;

  // Statistics
  volatile uint32_t _good_packets;
  volatile uint32_t _bad_packets;
  volatile uint32_t _clk_count;
  volatile uint32_t _cs_count;

  // ESP8266 register addresses for fast GPIO
#ifdef ESP8266
  static const uint32_t PIN_IN = 0x60000318;       // GPIO_IN register
  static const uint32_t PIN_OUT_SET = 0x60000304;  // GPIO_OUT_W1TS register
  static const uint32_t PIN_OUT_CLEAR = 0x60000308; // GPIO_OUT_W1TC register
#endif
};

// Global instance pointer for ISR access (TYPE1)
extern CioType1* g_cio_type1_instance;

}  // namespace bestway_spa
}  // namespace esphome
