/**
 * CIO TYPE1 Protocol Handler
 *
 * Adapted from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 * https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 * Original file: Code/lib/cio/CIO_TYPE1.cpp
 *
 * Original work Copyright (c) visualapproach
 * Licensed under GPL v3
 */

#include "CIO_TYPE1.h"

namespace bestway_va {

// Global instance pointer for ISR
CIO_TYPE1* g_cio_instance = nullptr;

// Static ISR wrappers
static void IRAM_ATTR isr_cs_wrapper() {
    if (g_cio_instance) g_cio_instance->isr_packetHandler();
}

static void IRAM_ATTR isr_clk_wrapper() {
    if (g_cio_instance) g_cio_instance->isr_clkHandler();
}

// 7-segment character codes
const uint8_t CIO_TYPE1::CHARCODES[38] = {
    0x7F, 0x0D, 0xB7, 0x9F, 0xCD, 0xDB, 0xFB, 0x0F, 0xFF, 0xDF,  // 0-9
    0x01, 0x81,  // space, dash
    0xEF, 0xF9, 0x73, 0xBD, 0xF3, 0xE3, 0xFB, 0xE9, 0xED, 0x61,  // A-K (partial)
    0x1D, 0xE1, 0x71, 0x01, 0xA9, 0xB9, 0xE7, 0xCF, 0xA1, 0xDB,  // L-U (partial)
    0xF1, 0x39, 0x7D, 0x01, 0xDD, 0xB7  // V-Z (partial)
};

const char CIO_TYPE1::CHARS[38] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ' ', '-',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

// PRE2021 button codes
const uint16_t CIO_PRE2021::_button_codes[11] = {
    0x1B1B,  // NOBTN
    0x0200,  // LOCK
    0x0100,  // TIMER
    0x0300,  // BUBBLES
    0x1012,  // UNIT
    0x1212,  // HEAT
    0x1112,  // PUMP
    0x1312,  // DOWN
    0x0809,  // UP
    0x0000,  // POWER (not available on PRE2021)
    0x0000   // HYDROJETS (not available on PRE2021)
};

uint16_t CIO_PRE2021::getButtonCode(Buttons button_index) {
    if (button_index >= BTN_COUNT) return _button_codes[NOBTN];
    return _button_codes[button_index];
}

CIO_TYPE1::CIO_TYPE1() {
    _byte_count = 0;
    _bit_count = 0;
    _data_is_output = false;
    _received_byte = 0;
    _CIO_cmd_matches = 0;
    _new_packet_available = false;
    _send_bit = 8;
    _brightness = 7;
    _packet_error = 0;
    _packet_transm_active = false;
    _button_code = 0x1B1B;  // NOBTN
}

void CIO_TYPE1::setup(int cio_data_pin, int cio_clk_pin, int cio_cs_pin) {
    g_cio_instance = this;
    _DATA_PIN = cio_data_pin;
    _CLK_PIN = cio_clk_pin;
    _CS_PIN = cio_cs_pin;
    _button_code = getButtonCode(NOBTN);

    pinMode(_CS_PIN, INPUT);
    pinMode(_DATA_PIN, INPUT);
    pinMode(_CLK_PIN, INPUT);

    attachInterrupt(digitalPinToInterrupt(_CS_PIN), isr_cs_wrapper, CHANGE);
    attachInterrupt(digitalPinToInterrupt(_CLK_PIN), isr_clk_wrapper, CHANGE);
}

void CIO_TYPE1::stop() {
    detachInterrupt(digitalPinToInterrupt(_CS_PIN));
    detachInterrupt(digitalPinToInterrupt(_CLK_PIN));
}

void CIO_TYPE1::pause_all(bool action) {
    if (action) {
        detachInterrupt(digitalPinToInterrupt(_CS_PIN));
        detachInterrupt(digitalPinToInterrupt(_CLK_PIN));
    } else {
        attachInterrupt(digitalPinToInterrupt(_CS_PIN), isr_cs_wrapper, CHANGE);
        attachInterrupt(digitalPinToInterrupt(_CLK_PIN), isr_clk_wrapper, CHANGE);
    }
}

void CIO_TYPE1::updateStates() {
    if (!_new_packet_available) return;
    _new_packet_available = false;

    if (_packet_error) {
        bad_packets_count++;
        packet_error = _packet_error;
        _packet_error = 0;
        return;
    }

    // Copy payload for DSP forwarding
    for (unsigned int i = 0; i < sizeof(_payload); i++) {
        _raw_payload_from_cio[i] = _payload[i];
    }
    good_packets_count++;
    brightness = _brightness & 7;

    // Parse status byte 7
    cio_states.locked = (_raw_payload_from_cio[LCK_IDX] & (1 << LCK_BIT)) > 0;
    cio_states.timerled1 = (_raw_payload_from_cio[TMR1_IDX] & (1 << TMR1_BIT)) > 0;
    cio_states.timerled2 = (_raw_payload_from_cio[TMR2_IDX] & (1 << TMR2_BIT)) > 0;
    cio_states.timerbuttonled = (_raw_payload_from_cio[TMRBTNLED_IDX] & (1 << TMRBTNLED_BIT)) > 0;
    cio_states.heatred = (_raw_payload_from_cio[REDHTR_IDX] & (1 << REDHTR_BIT)) > 0;
    cio_states.heatgrn = (_raw_payload_from_cio[GRNHTR_IDX] & (1 << GRNHTR_BIT)) > 0;
    cio_states.bubbles = (_raw_payload_from_cio[AIR_IDX] & (1 << AIR_BIT)) > 0;

    // Parse status byte 9
    cio_states.pump = (_raw_payload_from_cio[FLT_IDX] & (1 << FLT_BIT)) > 0;
    if (_raw_payload_from_cio[C_IDX] & (1 << C_BIT) || _raw_payload_from_cio[F_IDX] & (1 << F_BIT))
        cio_states.unit = (_raw_payload_from_cio[C_IDX] & (1 << C_BIT)) > 0 ? 0 : 1;
    cio_states.power = (_raw_payload_from_cio[PWR_IDX] & (1 << PWR_BIT)) > 0;
    if (getHasjets())
        cio_states.jets = (_raw_payload_from_cio[HJT_IDX] & (1 << HJT_BIT)) > 0;

    cio_states.heat = cio_states.heatgrn || cio_states.heatred;

    // Parse display characters
    cio_states.char1 = (uint8_t)_getChar(_raw_payload_from_cio[DGT1_IDX]);
    cio_states.char2 = (uint8_t)_getChar(_raw_payload_from_cio[DGT2_IDX]);
    cio_states.char3 = (uint8_t)_getChar(_raw_payload_from_cio[DGT3_IDX]);

    // Check for errors
    if (cio_states.char1 == 'E') {
        String errnum;
        errnum = (char)cio_states.char2;
        errnum += (char)cio_states.char3;
        cio_states.error = (uint8_t)(errnum.toInt());
    } else {
        cio_states.error = 0;
    }

    // Parse temperature if valid digits
    if (cio_states.char1 >= '0' && cio_states.char1 <= '9' &&
        cio_states.char2 >= '0' && cio_states.char2 <= '9') {
        String tempstring = String((char)cio_states.char1) +
                           String((char)cio_states.char2) +
                           String((char)cio_states.char3);
        uint8_t parsedValue = tempstring.toInt();
        if (parsedValue > 0 && parsedValue < 120) {
            cio_states.temperature = parsedValue;
        }
    }
}

void IRAM_ATTR CIO_TYPE1::eopHandler() {
#ifdef ESP8266
    WRITE_PERI_REG(PIN_DIR_INPUT, 1 << _DATA_PIN);
#else
    pinMode(_DATA_PIN, INPUT);
#endif
    if (_byte_count != 11 && _byte_count != 0) _packet_error |= 2;
    if (_bit_count != 0) _packet_error |= 1;
    _byte_count = 0;
    _bit_count = 0;
    uint8_t msg = _received_byte;

    switch (msg) {
        case DSP_CMD1_MODE6_11_7:
        case DSP_CMD1_MODE6_11_7_P05504:
            _CIO_cmd_matches = 1;
            break;
        case DSP_CMD2_DATAWRITE:
            if (_CIO_cmd_matches == 1) {
                _CIO_cmd_matches = 2;
            } else {
                _CIO_cmd_matches = 0;
            }
            break;
        default:
            if (_CIO_cmd_matches == 3) {
                _brightness = msg;
                _CIO_cmd_matches = 0;
                _new_packet_available = true;
            }
            if (_CIO_cmd_matches == 2) {
                _CIO_cmd_matches = 3;
            }
            break;
    }
}

void IRAM_ATTR CIO_TYPE1::isr_packetHandler() {
#ifdef ESP8266
    if ((READ_PERI_REG(PIN_IN) & (1 << _CS_PIN))) {
#else
    if (digitalRead(_CS_PIN)) {
#endif
        // End of packet (CS idle high)
        _packet_transm_active = false;
        _data_is_output = false;
        eopHandler();
    } else {
        // Packet start (CS active low)
        _packet_transm_active = true;
    }
}

void IRAM_ATTR CIO_TYPE1::isr_clkHandler() {
    if (!_packet_transm_active) return;

#ifdef ESP8266
    bool clockstate = READ_PERI_REG(PIN_IN) & (1 << _CLK_PIN);
#else
    bool clockstate = digitalRead(_CLK_PIN);
#endif

    // Falling edge: shift out button bits (per VA docs: "shift out on falling edge")
    // LSB first per VA docs
    if (!clockstate && _data_is_output) {
        button_bits_sent++;  // Debug: count bits transmitted
        if (_button_code & (1 << _send_bit)) {
#ifdef ESP8266
            WRITE_PERI_REG(PIN_OUT_SET, 1 << _DATA_PIN);
#else
            digitalWrite(_DATA_PIN, 1);
#endif
        } else {
#ifdef ESP8266
            WRITE_PERI_REG(PIN_OUT_CLEAR, 1 << _DATA_PIN);
#else
            digitalWrite(_DATA_PIN, 0);
#endif
        }
        _send_bit++;
        if (_send_bit > 15) _send_bit = 0;
    }

    // Rising edge: latch/read data bits (per VA docs: "latch on rising edge")
    if (clockstate && !_data_is_output) {
#ifdef ESP8266
        _received_byte = (_received_byte >> 1) | (((READ_PERI_REG(PIN_IN) & (1 << _DATA_PIN)) > 0) << 7);
#else
        _received_byte = (_received_byte >> 1) | (digitalRead(_DATA_PIN) << 7);
#endif
        _bit_count++;
        if (_bit_count == 8) {
            _bit_count = 0;

            // Store payload bytes
            if (_CIO_cmd_matches == 2) {
                if (_byte_count < 11) {
                    _payload[_byte_count] = _received_byte;
                    _byte_count++;
                } else {
                    _packet_error |= 4;
                }
            }
            // Check for button read request
            else if (_received_byte == DSP_CMD2_DATAREAD) {
                _data_is_output = true;
                cmd_read_count++;  // Debug: count 0x42 commands received
                last_btn_transmitted = _button_code;  // Debug: track what we're sending
#ifdef ESP8266
                WRITE_PERI_REG(PIN_DIR_OUTPUT, 1 << _DATA_PIN);
#else
                pinMode(_DATA_PIN, OUTPUT);
#endif
                // VA sends HIGH byte first: bits 8-15, then 0-7
                // Wait for first falling edge to output - do NOT output immediately
                _send_bit = 8;
            }
        }
    }
}

char CIO_TYPE1::_getChar(uint8_t value) {
    for (unsigned int index = 0; index < sizeof(CHARCODES); index++) {
        if ((value & 0xFE) == (CHARCODES[index] & 0xFE)) {
            return CHARS[index];
        }
    }
    return '*';
}

}  // namespace bestway_va
