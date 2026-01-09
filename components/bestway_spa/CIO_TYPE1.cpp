#include "CIO_TYPE1.h"
#include "Arduino.h"

// ESP8266 Register Macros for high-speed bit-banging
#ifndef READ_PERI_REG
#define READ_PERI_REG(addr) (*((volatile uint32_t *)(addr)))
#endif
#ifndef WRITE_PERI_REG
#define WRITE_PERI_REG(addr, val) (*((volatile uint32_t *)(addr))) = (uint32_t)(val)
#endif

// Register addresses for ESP8266
#define PIN_IN          0x60000318
#define PIN_OUT         0x60000300
#define PIN_OUT_SET     0x60000304
#define PIN_OUT_CLEAR   0x60000308

namespace esphome {
namespace bestway_spa {

// Lookup table for the 7-segment display characters
static const uint8_t CHARCODES[38] = {
    0x00, 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7C,
    0x39, 0x5E, 0x79, 0x71, 0x3D, 0x76, 0x06, 0x1E, 0x75, 0x38, 0x15, 0x37, 0x3F,
    0x73, 0x67, 0x50, 0xED, 0x78, 0x3E, 0x1C, 0x2A, 0x76, 0x6E, 0x5B, 0x40
};

static const char CHARS[38] = {
    ' ', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'b',
    'C', 'd', 'E', 'F', 'G', 'H', 'I', 'J', 'k', 'L', 'm', 'n', 'O',
    'P', 'q', 'r', 'S', 't', 'U', 'v', 'w', 'X', 'y', 'Z', '-'
};

void CIO_TYPE1::setup(uint8_t data_pin, uint8_t clk_pin, uint8_t cs_pin) {
  this->_DATA_PIN = data_pin;
  this->_CLK_PIN = clk_pin;
  this->_CS_PIN = cs_pin;

  pinMode(this->_CLK_PIN, INPUT);
  pinMode(this->_DATA_PIN, INPUT);
  pinMode(this->_CS_PIN, INPUT);

  // Initialize state
  this->_byte_count = 0;
  this->_received_byte = 0;
  this->_packet_transm_active = false;
  this->_data_is_output = false;
}

char CIO_TYPE1::_getChar(uint8_t code) {
  for (int i = 0; i < 38; i++) {
    if (CHARCODES[i] == (code & 0x7F)) {
      return CHARS[i];
    }
  }
  return '?';
}

void IRAM_ATTR CIO_TYPE1::isr_clkHandler() {
  uint32_t pins = READ_PERI_REG(PIN_IN);
  bool clk = pins & (1 << _CLK_PIN);
  bool data = pins & (1 << _DATA_PIN);
  bool cs = pins & (1 << _CS_PIN);

  if (cs) {
    this->eopHandler();
    return;
  }

  if (!this->_packet_transm_active) return;

  // Logic for reading/writing bits based on clock edges
  if (this->_data_is_output) {
    // Write bit to Data Pin
    if (_button_code & (1 << _send_bit)) {
      WRITE_PERI_REG(PIN_OUT_SET, (1 << _DATA_PIN));
    } else {
      WRITE_PERI_REG(PIN_OUT_CLEAR, (1 << _DATA_PIN));
    }
    _send_bit++;
  } else {
    // Read bit from Data Pin
    if (data) {
      this->_received_byte |= (1 << (7 - (_byte_count % 8)));
    }
    
    if (++_byte_count % 8 == 0) {
      this->_payload[(_byte_count / 8) - 1] = this->_received_byte;
      this->_received_byte = 0;
    }
  }
}

void IRAM_ATTR CIO_TYPE1::isr_packetHandler() {
  this->_packet_transm_active = true;
  this->_byte_count = 0;
  this->_received_byte = 0;
  this->_send_bit = 0;
}

void IRAM_ATTR CIO_TYPE1::eopHandler() {
  if (this->_packet_transm_active) {
    this->_new_packet_available = true;
    this->_packet_transm_active = false;
    this->good_packets_count++;
  }
}

void CIO_TYPE1::updateStates() {
  if (!this->_new_packet_available) return;

  // Example Payload Mapping (Adjust indices based on your specific T1 protocol)
  // Byte 0-2 often contain display segments
  this->_display_text[0] = _getChar(this->_payload[0]);
  this->_display_text[1] = _getChar(this->_payload[1]);
  this->_display_text[2] = _getChar(this->_payload[2]);
  this->_display_text[3] = '\0';

  // Byte 4/5 often contain temperature (this is a placeholder, needs your specific offset)
  this->_current_temp = (float)this->_payload[4];
  this->_target_temp = (float)this->_payload[5];

  // Bitmasks for status
  this->_heating = (this->_payload[3] & 0x01);
  this->_filtering = (this->_payload[3] & 0x02);
  this->_bubbles = (this->_payload[3] & 0x04);

  this->_new_packet_available = false;
}

}  // namespace bestway_spa
}  // namespace esphome
