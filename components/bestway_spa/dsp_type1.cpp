#include "dsp_type1.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bestway_spa {

static const char *const TAG = "dsp_type1";

// 7-segment character codes (38 characters)
const uint8_t DSP_TYPE1::CHARCODES[] = {
    0x7F, 0x0D, 0xB7, 0x9F, 0xCD, 0xDB, 0xFB, 0x0F, 0xFF, 0xDF,  // 0-9
    0xEF, 0xF9, 0x73, 0xBD, 0xF3, 0xE3, 0x7B, 0xE9, 0x09, 0x3D,  // A-J
    0xE1, 0x71, 0x49, 0xA9, 0xB9, 0xE7, 0xCF, 0xA1, 0xDB, 0xF1,  // K-T
    0x7D, 0x7D, 0x7D, 0xED, 0xDD, 0xB7, 0x00, 0x80               // U-Z, space, dash
};

const char DSP_TYPE1::CHARS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ -";

DSP_TYPE1::DSP_TYPE1() {
  // Initialize payload with command byte
  payload_[0] = DSP_CMD1_MODE6_11_7_P05504;
}

void DSP_TYPE1::setup(int data_pin, int clk_pin, int cs_pin, int audio_pin) {
  data_pin_ = data_pin;
  clk_pin_ = clk_pin;
  cs_pin_ = cs_pin;
  audio_pin_ = audio_pin;

  pinMode(cs_pin_, OUTPUT);
  pinMode(data_pin_, INPUT);
  pinMode(clk_pin_, OUTPUT);
  if (audio_pin_ >= 0) {
    pinMode(audio_pin_, OUTPUT);
    digitalWrite(audio_pin_, LOW);
  }

  // CS idle high
  digitalWrite(cs_pin_, HIGH);
  // Clock idle high
  digitalWrite(clk_pin_, HIGH);

  ESP_LOGI(TAG, "DSP TYPE1 initialized - CLK:%d DATA:%d CS:%d AUDIO:%d",
           clk_pin_, data_pin_, cs_pin_, audio_pin_);
}

void DSP_TYPE1::stop() {
  if (audio_pin_ >= 0) {
    noTone(audio_pin_);
  }
}

void DSP_TYPE1::send_bits_to_dsp(uint32_t out_bits, int bits_to_send) {
  pinMode(data_pin_, OUTPUT);
  delayMicroseconds(20);

  for (int i = 0; i < bits_to_send; i++) {
    digitalWrite(clk_pin_, LOW);
    digitalWrite(data_pin_, (out_bits & (1 << i)) ? HIGH : LOW);
    delayMicroseconds(20);
    digitalWrite(clk_pin_, HIGH);
    delayMicroseconds(20);
  }
}

uint16_t DSP_TYPE1::receive_bits_from_dsp() {
  uint16_t result = 0;
  pinMode(data_pin_, INPUT);

  for (int i = 0; i < 16; i++) {
    digitalWrite(clk_pin_, LOW);
    delayMicroseconds(20);
    digitalWrite(clk_pin_, HIGH);
    delayMicroseconds(20);
    int j = (i + 8) % 16;  // bit 8-15 then 0-7
    result |= (digitalRead(data_pin_) << j);
  }
  return result;
}

void DSP_TYPE1::clear_payload() {
  for (unsigned int i = 1; i < sizeof(payload_); i++) {
    payload_[i] = 0;
  }
}

uint8_t DSP_TYPE1::char_to_7seg_code(char c) {
  for (unsigned int i = 0; i < sizeof(CHARS); i++) {
    if (c == CHARS[i]) {
      return CHARCODES[i];
    }
  }
  return 0x00;  // space
}

Buttons DSP_TYPE1::button_code_to_index(uint16_t code) {
  for (uint8_t i = 0; i < BTN_COUNT; i++) {
    if (button_codes_[i] == code) {
      return static_cast<Buttons>(i);
    }
  }
  return NOBTN;
}

Buttons DSP_TYPE1::get_pressed_button() {
  if (millis() - last_button_poll_time_ < 90) {
    return old_button_;
  }

  last_button_poll_time_ = millis();

  // Start packet
  digitalWrite(cs_pin_, LOW);
  delayMicroseconds(50);

  // Request button state
  send_bits_to_dsp(DSP_CMD2_DATAREAD, 8);
  uint16_t button_code = receive_bits_from_dsp();

  // End packet
  digitalWrite(cs_pin_, HIGH);
  delayMicroseconds(30);

  raw_payload_from_dsp_[0] = button_code >> 8;
  raw_payload_from_dsp_[1] = button_code & 0xFF;

  Buttons new_button = button_code_to_index(button_code);
  old_button_ = new_button;

  return new_button;
}

void DSP_TYPE1::handle_states() {
  // Set display characters
  if (text_.length() > 0) {
    payload_[DGT1_IDX] = char_to_7seg_code(text_[0]);
    payload_[DGT2_IDX] = (text_.length() > 1) ? char_to_7seg_code(text_[1]) : 0x01;
    payload_[DGT3_IDX] = (text_.length() > 2) ? char_to_7seg_code(text_[2]) : 0x01;
  } else {
    payload_[DGT1_IDX] = char_to_7seg_code(dsp_states_.char1);
    payload_[DGT2_IDX] = char_to_7seg_code(dsp_states_.char2);
    payload_[DGT3_IDX] = char_to_7seg_code(dsp_states_.char3);
  }

  if (dsp_states_.power) {
    // Lock LED
    payload_[LCK_IDX] &= ~(1 << LCK_BIT);
    payload_[LCK_IDX] |= dsp_states_.locked << LCK_BIT;

    // Timer button LED
    payload_[TMRBTNLED_IDX] &= ~(1 << TMRBTNLED_BIT);
    payload_[TMRBTNLED_IDX] |= dsp_states_.timerbuttonled << TMRBTNLED_BIT;

    // Timer LEDs
    payload_[TMR1_IDX] &= ~(1 << TMR1_BIT);
    payload_[TMR1_IDX] |= dsp_states_.timerled1 << TMR1_BIT;
    payload_[TMR2_IDX] &= ~(1 << TMR2_BIT);
    payload_[TMR2_IDX] |= dsp_states_.timerled2 << TMR2_BIT;

    // Heater LEDs
    payload_[REDHTR_IDX] &= ~(1 << REDHTR_BIT);
    payload_[REDHTR_IDX] |= dsp_states_.heatred << REDHTR_BIT;
    payload_[GRNHTR_IDX] &= ~(1 << GRNHTR_BIT);
    payload_[GRNHTR_IDX] |= dsp_states_.heatgrn << GRNHTR_BIT;

    // Bubbles LED
    payload_[AIR_IDX] &= ~(1 << AIR_BIT);
    payload_[AIR_IDX] |= dsp_states_.bubbles << AIR_BIT;

    // Filter/Pump LED
    payload_[FLT_IDX] &= ~(1 << FLT_BIT);
    payload_[FLT_IDX] |= dsp_states_.pump << FLT_BIT;

    // Unit LEDs (C/F)
    payload_[C_IDX] &= ~(1 << C_BIT);
    payload_[C_IDX] |= dsp_states_.unit << C_BIT;
    payload_[F_IDX] &= ~(1 << F_BIT);
    payload_[F_IDX] |= (!dsp_states_.unit) << F_BIT;

    // Power LED
    payload_[PWR_IDX] &= ~(1 << PWR_BIT);
    payload_[PWR_IDX] |= dsp_states_.power << PWR_BIT;

    // Jets LED
    payload_[HJT_IDX] &= ~(1 << HJT_BIT);
    payload_[HJT_IDX] |= dsp_states_.jets << HJT_BIT;
  } else {
    clear_payload();
  }

  // Handle audio
  if (audio_pin_ >= 0) {
    if (audio_frequency_ > 0) {
      tone(audio_pin_, audio_frequency_);
    } else {
      noTone(audio_pin_);
    }
  }

  upload_payload(dsp_states_.brightness);
}

void DSP_TYPE1::upload_payload(uint8_t brightness) {
  // Refresh at ~20Hz
  if (millis() - last_refresh_time_ < 50) {
    return;
  }
  last_refresh_time_ = millis();

  uint8_t enable_led = 0;
  if (brightness > 0) {
    enable_led = DSP_DIM_ON;
    brightness -= 1;
  }

  // Packet 1: Mode command
  delayMicroseconds(30);
  digitalWrite(cs_pin_, LOW);
  send_bits_to_dsp(DSP_CMD1_MODE6_11_7_P05504, 8);
  digitalWrite(cs_pin_, HIGH);

  // Packet 2: Data write command
  delayMicroseconds(50);
  digitalWrite(cs_pin_, LOW);
  send_bits_to_dsp(DSP_CMD2_DATAWRITE, 8);
  digitalWrite(cs_pin_, HIGH);

  // Packet 3: Payload (11 bytes)
  delayMicroseconds(50);
  digitalWrite(cs_pin_, LOW);
  for (int i = 0; i < 11; i++) {
    send_bits_to_dsp(payload_[i], 8);
  }
  digitalWrite(cs_pin_, HIGH);

  // Packet 4: Brightness
  delayMicroseconds(50);
  digitalWrite(cs_pin_, LOW);
  send_bits_to_dsp(DSP_DIM_BASE | enable_led | brightness, 8);
  digitalWrite(cs_pin_, HIGH);
  delayMicroseconds(50);
}

}  // namespace bestway_spa
}  // namespace esphome
