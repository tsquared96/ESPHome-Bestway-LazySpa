#include "CIO_TYPE1.h"

namespace bestway_va {

CIO_TYPE1* g_cio_instance = nullptr;

static void IRAM_ATTR isr_cs_wrapper() { if (g_cio_instance) g_cio_instance->isr_packetHandler(); }
static void IRAM_ATTR isr_clk_wrapper() { if (g_cio_instance) g_cio_instance->isr_clkHandler(); }

const uint8_t CIO_TYPE1::CHARCODES[38] = {
    0x7F, 0x0D, 0xB7, 0x9F, 0xCD, 0xDB, 0xFB, 0x0F, 0xFF, 0xDF,
    0x01, 0x81, 0xEF, 0xF9, 0x73, 0xBD, 0xF3, 0xE3, 0xFB, 0xE9, 
    0xED, 0x61, 0x1D, 0xE1, 0x71, 0x01, 0xA9, 0xB9, 0xE7, 0xCF, 
    0xA1, 0xDB, 0xF1, 0x39, 0x7D, 0x01, 0xDD, 0xB7
};

const char CIO_TYPE1::CHARS[38] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ' ', '-',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

const uint16_t CIO_PRE2021::_button_codes[11] = {
    0x1B1B, 0x0200, 0x0100, 0x0300, 0x1012, 0x1212, 0x1112, 0x1312, 0x0809, 0x0000, 0x0000
};

uint16_t CIO_PRE2021::getButtonCode(Buttons button_index) {
    if (button_index >= BTN_COUNT) return _button_codes[NOBTN];
    return _button_codes[button_index];
}

CIO_TYPE1::CIO_TYPE1() {
    _byte_count = 0; _bit_count = 0; _data_is_output = false; _received_byte = 0;
    _CIO_cmd_matches = 0; _new_packet_available = false; _send_bit = 8;
    _brightness = 7; _packet_error = 0; _packet_transm_active = false;
    _button_code = 0x1B1B;
}

void CIO_TYPE1::setup(int cio_data_pin, int cio_clk_pin, int cio_cs_pin) {
    g_cio_instance = this;
    _DATA_PIN = cio_data_pin; _CLK_PIN = cio_clk_pin; _CS_PIN = cio_cs_pin;
    pinMode(_CS_PIN, INPUT); pinMode(_DATA_PIN, INPUT); pinMode(_CLK_PIN, INPUT);

    attachInterrupt(digitalPinToInterrupt(_CS_PIN), isr_cs_wrapper, CHANGE);
    attachInterrupt(digitalPinToInterrupt(_CLK_PIN), isr_clk_wrapper, CHANGE);
}

void IRAM_ATTR CIO_TYPE1::isr_clkHandler() {
    if (!_packet_transm_active) return;
    uint32_t gpio_in = READ_PERI_REG(PIN_IN);
    bool clockstate = gpio_in & (1 << _CLK_PIN);

    if (!clockstate && _data_is_output) {
        if (_button_code & (1 << _send_bit)) WRITE_PERI_REG(PIN_OUT_SET, 1 << _DATA_PIN);
        else WRITE_PERI_REG(PIN_OUT_CLEAR, 1 << _DATA_PIN);
        _send_bit++; if (_send_bit > 15) _send_bit = 0;
        return;
    }

    if (clockstate && !_data_is_output) {
        _received_byte = (_received_byte >> 1) | (((gpio_in & (1 << _DATA_PIN)) > 0) << 7);
        _bit_count++;
        if (_bit_count == 8) {
            _bit_count = 0;
            if (_CIO_cmd_matches == 2 && _byte_count < 11) {
                _payload[_byte_count++] = _received_byte;
            } else if (_received_byte == DSP_CMD2_DATAREAD) {
                _send_bit = 8; _data_is_output = true;
                WRITE_PERI_REG(PIN_DIR_OUTPUT, 1 << _DATA_PIN);
                if (_button_code & (1 << _send_bit)) WRITE_PERI_REG(PIN_OUT_SET, 1 << _DATA_PIN);
                else WRITE_PERI_REG(PIN_OUT_CLEAR, 1 << _DATA_PIN);
                _send_bit++; cmd_read_count++;
            }
        }
    }
}

void IRAM_ATTR CIO_TYPE1::isr_packetHandler() {
    if (READ_PERI_REG(PIN_IN) & (1 << _CS_PIN)) {
        _packet_transm_active = false; _data_is_output = false; eopHandler();
    } else { _packet_transm_active = true; }
}

void IRAM_ATTR CIO_TYPE1::eopHandler() {
    WRITE_PERI_REG(PIN_DIR_INPUT, 1 << _DATA_PIN);
    if (_byte_count != 11 && _byte_count != 0) _packet_error |= 2;
    if (_bit_count != 0) _packet_error |= 1;
    uint8_t msg = _received_byte;
    _byte_count = 0; _bit_count = 0;
    switch (msg) {
        case DSP_CMD1_MODE6_11_7: case DSP_CMD1_MODE6_11_7_P05504: _CIO_cmd_matches = 1; break;
        case DSP_CMD2_DATAWRITE: if (_CIO_cmd_matches == 1) _CIO_cmd_matches = 2; else _CIO_cmd_matches = 0; break;
        default:
            if (_CIO_cmd_matches == 3) { _brightness = msg; _CIO_cmd_matches = 0; _new_packet_available = true; }
            if (_CIO_cmd_matches == 2) _CIO_cmd_matches = 3;
            break;
    }
}

void CIO_TYPE1::updateStates() {
    if (!_new_packet_available) return;
    _new_packet_available = false;
    if (_packet_error) { bad_packets_count++; _packet_error = 0; return; }
    good_packets_count++;
    brightness = _brightness & 7;

    cio_states.locked = (_payload[7] & (1 << 3)) > 0;
    cio_states.heatred = (_payload[7] & (1 << 5)) > 0;
    cio_states.heatgrn = (_payload[7] & (1 << 6)) > 0;
    cio_states.bubbles = (_payload[7] & (1 << 7)) > 0;
    cio_states.pump = (_payload[9] & (1 << 1)) > 0;
    cio_states.power = (_payload[9] & (1 << 4)) > 0;
    cio_states.heat = cio_states.heatgrn || cio_states.heatred;
    cio_states.char1 = (uint8_t)_getChar(_payload[1]);
    cio_states.char2 = (uint8_t)_getChar(_payload[3]);
    cio_states.char3 = (uint8_t)_getChar(_payload[5]);
    
    if (cio_states.char1 >= '0' && cio_states.char1 <= '9') {
        String ts = String((char)cio_states.char1) + String((char)cio_states.char2);
        cio_states.temperature = ts.toInt();
    }
}

char CIO_TYPE1::_getChar(uint8_t value) {
    for (int i = 0; i < 38; i++) if ((value & 0xFE) == (CHARCODES[i] & 0xFE)) return CHARS[i];
    return '*';
}

void CIO_TYPE1::stop() { detachInterrupt(digitalPinToInterrupt(_CS_PIN)); detachInterrupt(digitalPinToInterrupt(_CLK_PIN)); }
}
