#pragma once
/**
 * CIO TYPE2 Protocol Handler - Ported from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 *
 * For models: 54149E
 *
 * Protocol: 6-wire SPI-like, different from TYPE1
 * - 5-byte payload structure
 * - Different LED/status bit positions
 * - Different button code format
 *
 * Based on: https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 * Original author: visualapproach
 */

#include <Arduino.h>
#include <stdint.h>

namespace esphome {
namespace bestway_spa {

// =============================================================================
// CIO TYPE2 Constants - From VisualApproach
// =============================================================================

// CIO commands for TYPE2
static const uint8_t CIO_CMD_BEGIN_TYPE2 = 0x01;
static const uint8_t CIO_CMD_END_TYPE2 = 0x00;
static const uint8_t CIO_CMD_GETBUTTONS_TYPE2 = 0x10;

// LED bit masks for TYPE2 (byte 4 = LEDs)
// From VisualApproach CIO_TYPE2.h
static const uint8_t LED_POWER_TYPE2   = 0x01;  // Power LED
static const uint8_t LED_BUBBLES_TYPE2 = 0x02;  // Bubbles/Air
static const uint8_t LED_JETS_TYPE2    = 0x04;  // Jets
static const uint8_t LED_HEATGRN_TYPE2 = 0x08;  // Heat standby (green)
static const uint8_t LED_HEATRED_TYPE2 = 0x10;  // Heat active (red)
static const uint8_t LED_PUMP_TYPE2    = 0x20;  // Filter pump
static const uint8_t LED_UNIT_F_TYPE2  = 0x40;  // Fahrenheit (0=Celsius)
static const uint8_t LED_LOCK_TYPE2    = 0x80;  // Lock

// 7-segment character codes for TYPE2
static const uint8_t CHARCODES_TYPE2_DISP[] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F,  // 0-9
  0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71, 0x3D, 0x76, 0x06, 0x0E,  // A-J
  0x70, 0x38, 0x15, 0x54, 0x5C, 0x73, 0x67, 0x50, 0x6D, 0x78,  // K-T
  0x3E, 0x3E, 0x3E, 0x76, 0x6E, 0x5B, 0x00, 0x40              // U-Z, space, dash
};

// Character lookup
static const char CHARS_TYPE2[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ -";

// =============================================================================
// CIO TYPE2 Class - Ported from VisualApproach
// =============================================================================

class CioType2 {
 public:
  CioType2();

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

  // ISR handlers
  void IRAM_ATTR isr_csHandler();
  void IRAM_ATTR isr_clkHandler();

 private:
  // Pin numbers
  int _cio_clk_pin;
  int _cio_data_pin;
  int _cio_cs_pin;
  int _dsp_data_pin;

  // Packet buffer (5 bytes for TYPE2)
  volatile uint8_t _packet[5];
  volatile uint8_t _packet_ready[5];
  volatile bool _new_packet;
  volatile uint8_t _bit_count;
  volatile uint8_t _byte_count;
  volatile uint8_t _current_byte;

  // Button transmission
  volatile uint16_t _button_code;
  volatile bool _sending_buttons;
  volatile uint8_t _send_bit;

  // Protocol state
  volatile bool _packet_active;
  volatile uint8_t _cmd_byte;  // Command byte from CIO

  // Statistics
  volatile uint32_t _good_packets;
  volatile uint32_t _bad_packets;
  volatile uint32_t _clk_count;
  volatile uint32_t _cs_count;

  // ESP8266 register addresses
#ifdef ESP8266
  static const uint32_t PIN_IN = 0x60000318;
  static const uint32_t PIN_OUT_SET = 0x60000304;
  static const uint32_t PIN_OUT_CLEAR = 0x60000308;
#endif
};

// Global instance pointer for ISR access (TYPE2)
extern CioType2* g_cio_type2_instance;

}  // namespace bestway_spa
}  // namespace esphome
