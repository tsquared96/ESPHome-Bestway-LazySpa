#include "cio_type1.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bestway_spa {

static const char *const TAG = "cio_type1";

// Global pointer for ISR
CIO_TYPE1 *g_cio_type1_instance = nullptr;

// 7-segment character codes (38 characters)
const uint8_t CIO_TYPE1::CHARCODES[] = {
    0x7F, 0x0D, 0xB7, 0x9F, 0xCD, 0xDB, 0xFB, 0x0F, 0xFF, 0xDF,  // 0-9
    0xEF, 0xF9, 0x73, 0xBD, 0xF3, 0xE3, 0x7B, 0xE9, 0x09, 0x3D,  // A-J
    0xE1, 0x71, 0x49, 0xA9, 0xB9, 0xE7, 0xCF, 0xA1, 0xDB, 0xF1,  // K-T
    0x7D, 0x7D, 0x7D, 0xED, 0xDD, 0xB7, 0x00, 0x80               // U-Z, space, dash
};

const char CIO_TYPE1::CHARS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ -";

// ISR wrapper functions
static void IRAM_ATTR isr_cs_wrapper() {
  if (g_cio_type1_instance != nullptr) {
    g_cio_type1_instance->isr_packet_handler();
  }
}

static void IRAM_ATTR isr_clk_wrapper() {
  if (g_cio_type1_instance != nullptr) {
    g_cio_type1_instance->isr_clk_handler();
  }
}

CIO_TYPE1::CIO_TYPE1() {
  byte_count_ = 0;
  bit_count_ = 0;
  data_is_output_ = false;
  received_byte_ = 0;
  cio_cmd_matches_ = 0;
  new_packet_available_ = false;
  send_bit_ = 8;
  brightness_ = 7;
  packet_error_ = 0;
}

void CIO_TYPE1::setup(int data_pin, int clk_pin, int cs_pin) {
  g_cio_type1_instance = this;
  data_pin_ = data_pin;
  clk_pin_ = clk_pin;
  cs_pin_ = cs_pin;

  button_code_ = get_button_code(NOBTN);

  // Set pin modes - all inputs initially
  pinMode(cs_pin_, INPUT);
  pinMode(data_pin_, INPUT);
  pinMode(clk_pin_, INPUT);

  // Get interrupt numbers
  int cs_int = digitalPinToInterrupt(cs_pin_);
  int clk_int = digitalPinToInterrupt(clk_pin_);

  ESP_LOGI(TAG, "CIO TYPE1 setup - CLK:GPIO%d(int%d) DATA:GPIO%d CS:GPIO%d(int%d)",
           clk_pin_, clk_int, data_pin_, cs_pin_, cs_int);

  // Attach interrupts
  if (cs_int >= 0) {
    attachInterrupt(cs_int, isr_cs_wrapper, CHANGE);
    ESP_LOGI(TAG, "CS interrupt attached");
  } else {
    ESP_LOGE(TAG, "CS pin %d does not support interrupts!", cs_pin_);
  }

  if (clk_int >= 0) {
    attachInterrupt(clk_int, isr_clk_wrapper, CHANGE);
    ESP_LOGI(TAG, "CLK interrupt attached");
  } else {
    ESP_LOGE(TAG, "CLK pin %d does not support interrupts!", clk_pin_);
  }

  ESP_LOGI(TAG, "CIO TYPE1 initialized - waiting for data from spa controller");
}

void CIO_TYPE1::stop() {
  detachInterrupt(digitalPinToInterrupt(cs_pin_));
  detachInterrupt(digitalPinToInterrupt(clk_pin_));
  g_cio_type1_instance = nullptr;
}

void CIO_TYPE1::pause(bool action) {
  if (action) {
    detachInterrupt(digitalPinToInterrupt(cs_pin_));
    detachInterrupt(digitalPinToInterrupt(clk_pin_));
  } else {
    attachInterrupt(digitalPinToInterrupt(cs_pin_), isr_cs_wrapper, CHANGE);
    attachInterrupt(digitalPinToInterrupt(clk_pin_), isr_clk_wrapper, CHANGE);
  }
}

void CIO_TYPE1::update_states() {
  if (!new_packet_available_)
    return;

  new_packet_available_ = false;

  if (packet_error_) {
    bad_packets_count_++;
    packet_error_ = 0;
    return;
  }

  // Copy payload
  for (unsigned int i = 0; i < sizeof(payload_); i++) {
    raw_payload_from_cio_[i] = payload_[i];
  }
  good_packets_count_++;

  // Parse states
  cio_states_.locked = (raw_payload_from_cio_[LCK_IDX] & (1 << LCK_BIT)) > 0;
  cio_states_.power = (raw_payload_from_cio_[PWR_IDX] & (1 << PWR_BIT)) > 0;

  // Unit (C/F)
  if ((raw_payload_from_cio_[C_IDX] & (1 << C_BIT)) || (raw_payload_from_cio_[F_IDX] & (1 << F_BIT))) {
    cio_states_.unit = (raw_payload_from_cio_[C_IDX] & (1 << C_BIT)) > 0;
  }

  cio_states_.bubbles = (raw_payload_from_cio_[AIR_IDX] & (1 << AIR_BIT)) > 0;
  cio_states_.heatgrn = (raw_payload_from_cio_[GRNHTR_IDX] & (1 << GRNHTR_BIT)) > 0;
  cio_states_.heatred = (raw_payload_from_cio_[REDHTR_IDX] & (1 << REDHTR_BIT)) > 0;
  cio_states_.heat = cio_states_.heatgrn || cio_states_.heatred;
  cio_states_.pump = (raw_payload_from_cio_[FLT_IDX] & (1 << FLT_BIT)) > 0;
  cio_states_.timerled1 = (raw_payload_from_cio_[TMR1_IDX] & (1 << TMR1_BIT)) > 0;
  cio_states_.timerled2 = (raw_payload_from_cio_[TMR2_IDX] & (1 << TMR2_BIT)) > 0;
  cio_states_.timerbuttonled = (raw_payload_from_cio_[TMRBTNLED_IDX] & (1 << TMRBTNLED_BIT)) > 0;

  // Decode display characters
  cio_states_.char1 = get_char(raw_payload_from_cio_[DGT1_IDX]);
  cio_states_.char2 = get_char(raw_payload_from_cio_[DGT2_IDX]);
  cio_states_.char3 = get_char(raw_payload_from_cio_[DGT3_IDX]);

  if (has_jets_) {
    cio_states_.jets = (raw_payload_from_cio_[HJT_IDX] & (1 << HJT_BIT)) > 0;
  } else {
    cio_states_.jets = false;
  }

  // Parse temperature from display
  if (cio_states_.char1 != '*' && cio_states_.char2 != '*' && cio_states_.char3 != '*') {
    if (cio_states_.char1 == 'E') {
      // Error code
      if (cio_states_.char2 >= '0' && cio_states_.char2 <= '9' &&
          cio_states_.char3 >= '0' && cio_states_.char3 <= '9') {
        cio_states_.error = (cio_states_.char2 - '0') * 10 + (cio_states_.char3 - '0');
      }
    } else if (cio_states_.char3 != 'H' && cio_states_.char3 != ' ') {
      cio_states_.error = 0;
      // Try to parse temperature
      if (cio_states_.char1 >= '0' && cio_states_.char1 <= '9' &&
          cio_states_.char2 >= '0' && cio_states_.char2 <= '9' &&
          cio_states_.char3 >= '0' && cio_states_.char3 <= '9') {
        uint8_t temp = (cio_states_.char1 - '0') * 100 +
                       (cio_states_.char2 - '0') * 10 +
                       (cio_states_.char3 - '0');
        if (temp > 0 && temp < 150) {
          cio_states_.temperature = temp;
        }
      } else if (cio_states_.char1 == ' ' &&
                 cio_states_.char2 >= '0' && cio_states_.char2 <= '9' &&
                 cio_states_.char3 >= '0' && cio_states_.char3 <= '9') {
        uint8_t temp = (cio_states_.char2 - '0') * 10 + (cio_states_.char3 - '0');
        if (temp > 0 && temp < 150) {
          cio_states_.temperature = temp;
        }
      }
    }
  }
}

void IRAM_ATTR CIO_TYPE1::eop_handler() {
  // End of packet - set data pin to input
#ifdef ESP8266
  GPEC = (1 << data_pin_);  // GPIO Enable Clear - set as input
#else
  pinMode(data_pin_, INPUT);
#endif

  if (byte_count_ != 11 && byte_count_ != 0)
    packet_error_ |= 2;
  if (bit_count_ != 0)
    packet_error_ |= 1;

  byte_count_ = 0;
  bit_count_ = 0;
  uint8_t msg = received_byte_;

  switch (msg) {
    case DSP_CMD1_MODE6_11_7:
    case DSP_CMD1_MODE6_11_7_P05504:
      cio_cmd_matches_ = 1;
      break;
    case DSP_CMD2_DATAWRITE:
      if (cio_cmd_matches_ == 1) {
        cio_cmd_matches_ = 2;
      } else {
        cio_cmd_matches_ = 0;
      }
      break;
    default:
      if (cio_cmd_matches_ == 3) {
        brightness_ = msg;
        cio_cmd_matches_ = 0;
        new_packet_available_ = true;
      }
      if (cio_cmd_matches_ == 2) {
        cio_cmd_matches_ = 3;
      }
      break;
  }
}

void IRAM_ATTR CIO_TYPE1::isr_packet_handler() {
  cs_interrupt_count_++;  // Debug counter

#ifdef ESP8266
  if (GPI & (1 << cs_pin_)) {  // GPIO Input register
#else
  if (digitalRead(cs_pin_)) {
#endif
    // CS high = end of packet
    packet_transm_active_ = false;
    data_is_output_ = false;
    eop_handler();
  } else {
    // CS low = start of packet
    packet_transm_active_ = true;
  }
}

void IRAM_ATTR CIO_TYPE1::isr_clk_handler() {
  clk_interrupt_count_++;  // Debug counter

  // Fallback: Poll CS state if CS interrupt isn't working
  // Check CS state on every clock edge
#ifdef ESP8266
  bool cs_high = GPI & (1 << cs_pin_);
#else
  bool cs_high = digitalRead(cs_pin_);
#endif

  // Detect CS transitions by comparing with packet_transm_active state
  if (!cs_high && !packet_transm_active_) {
    // CS went LOW - start of packet
    packet_transm_active_ = true;
    cs_interrupt_count_++;  // Count this as a virtual CS interrupt
  } else if (cs_high && packet_transm_active_) {
    // CS went HIGH - end of packet
    packet_transm_active_ = false;
    data_is_output_ = false;
    eop_handler();
    return;  // Don't process this clock edge
  }

  if (!packet_transm_active_)
    return;

#ifdef ESP8266
  bool clockstate = GPI & (1 << clk_pin_);  // GPIO Input register
#else
  bool clockstate = digitalRead(clk_pin_);
#endif

  // Shift out bits on falling edge (clock low)
  if (!clockstate && data_is_output_) {
    if (button_code_ & (1 << send_bit_)) {
#ifdef ESP8266
      GPOS = (1 << data_pin_);  // GPIO Output Set
#else
      digitalWrite(data_pin_, HIGH);
#endif
    } else {
#ifdef ESP8266
      GPOC = (1 << data_pin_);  // GPIO Output Clear
#else
      digitalWrite(data_pin_, LOW);
#endif
    }
    send_bit_++;
    if (send_bit_ > 15)
      send_bit_ = 0;
  }

  // Read bits on rising edge (clock high)
  if (clockstate && !data_is_output_) {
#ifdef ESP8266
    received_byte_ = (received_byte_ >> 1) | (((GPI & (1 << data_pin_)) > 0) << 7);
#else
    received_byte_ = (received_byte_ >> 1) | (digitalRead(data_pin_) << 7);
#endif
    bit_count_++;

    if (bit_count_ == 8) {
      bit_count_ = 0;

      if (cio_cmd_matches_ == 2) {
        if (byte_count_ < 11) {
          payload_[byte_count_] = received_byte_;
          byte_count_++;
        } else {
          packet_error_ |= 4;
        }
      } else if (received_byte_ == DSP_CMD2_DATAREAD) {
        send_bit_ = 8;
        data_is_output_ = true;
#ifdef ESP8266
        GPES = (1 << data_pin_);  // GPIO Enable Set - set as output
#else
        pinMode(data_pin_, OUTPUT);
#endif
      }
    }
  }
}

char CIO_TYPE1::get_char(uint8_t value) {
  for (unsigned int i = 0; i < sizeof(CHARCODES); i++) {
    if ((value & 0xFE) == (CHARCODES[i] & 0xFE)) {
      return CHARS[i];
    }
  }
  return '*';
}

uint16_t CIO_TYPE1::get_button_code(Buttons button) {
  if (button < BTN_COUNT) {
    return button_codes_[button];
  }
  return button_codes_[NOBTN];
}

Buttons CIO_TYPE1::get_button_from_code(uint16_t code) {
  for (uint8_t i = 0; i < BTN_COUNT; i++) {
    if (button_codes_[i] == code) {
      return static_cast<Buttons>(i);
    }
  }
  return NOBTN;
}

void CIO_TYPE1::set_button_codes(const uint16_t *codes, size_t count) {
  for (size_t i = 0; i < count && i < BTN_COUNT; i++) {
    button_codes_[i] = codes[i];
  }
}

}  // namespace bestway_spa
}  // namespace esphome
