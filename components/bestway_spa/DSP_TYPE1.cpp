#include "DSP_TYPE1.h"
#include "Arduino.h"

// ESP8266 Register Macros for high-speed bit-banging
#ifndef READ_PERI_REG
#define READ_PERI_REG(addr) (*((volatile uint32_t *)(addr)))
#endif

#define PIN_IN 0x60000318

namespace esphome {
namespace bestway_spa {

void DSP_TYPE1::setup(uint8_t data_pin, uint8_t clk_pin, uint8_t cs_pin, uint8_t audio_pin) {
  this->_DATA_PIN = data_pin;
  this->_CLK_PIN = clk_pin;
  this->_CS_PIN = cs_pin;
  this->_AUDIO_PIN = audio_pin;

  pinMode(this->_CLK_PIN, INPUT);
  pinMode(this->_DATA_PIN, INPUT);
  pinMode(this->_CS_PIN, INPUT);
  if (this->_AUDIO_PIN != 255) {
    pinMode(this->_AUDIO_PIN, INPUT);
  }

  this->_byte_count = 0;
  this->_bit_count = 0;
  this->_received_byte = 0;
  this->_packet_transm_active = false;
  this->_new_packet_available = false;
}

void IRAM_ATTR DSP_TYPE1::isr_clkHandler() {
  uint32_t pins = READ_PERI_REG(PIN_IN);
  bool clk = pins & (1 << _CLK_PIN);
  bool data = pins & (1 << _DATA_PIN);
  bool cs = pins & (1 << _CS_PIN);

  if (cs) {
    if (this->_packet_transm_active) {
      this->_new_packet_available = true;
      this->_packet_transm_active = false;
    }
    return;
  }

  if (!this->_packet_transm_active) {
    this->_packet_transm_active = true;
    this->_byte_count = 0;
    this->_bit_count = 0;
    this->_received_byte = 0;
  }

  if (data) {
    this->_received_byte |= (1 << (7 - this->_bit_count));
  }

  this->_bit_count++;

  if (this->_bit_count >= 8) {
    if (this->_byte_count < 11) {
      this->_payload[this->_byte_count] = this->_received_byte;
    }
    this->_byte_count++;
    this->_bit_count = 0;
    this->_received_byte = 0;
  }
}

uint8_t DSP_TYPE1::getPressedButton() {
  uint8_t btn = 0;
  if (this->_byte_count >= 7) {
    btn = this->_payload[6];
  }

  if (btn != this->_last_btn) {
    this->_last_btn = btn;
    return btn;
  }
  return 0;
}

}  // namespace bestway_spa
}  // namespace esphome
