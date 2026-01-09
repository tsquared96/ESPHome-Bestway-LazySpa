#include "CIO_TYPE1.h"

namespace bestway_va {

// 1. Static Data Definitions
const uint8_t CIO_TYPE1::CHARCODES[38] = {
    0x7E, 0x30, 0x6D, 0x79, 0x33, 0x5B, 0x5F, 0x70, 0x7F, 0x7B,
    0x77, 0x1F, 0x4E, 0x3D, 0x4F, 0x47, 0x5E, 0x37, 0x06, 0x3C,
    0x57, 0x0E, 0x54, 0x15, 0x1D, 0x67, 0x73, 0x05, 0x5B, 0x0F,
    0x3E, 0x1C, 0x5C, 0x49, 0x3B, 0x25, 0x00, 0x40
};

const char CIO_TYPE1::CHARS[38] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z', ' ', '-'
};

const uint16_t CIO_PRE2021::_button_codes[11] = {
    0x1B1B, 0x1B11, 0x1B0B, 0x1B17, 0x1B1D, 0x1B1E, 0x1B1D, 0x0B1B, 0x1B1B, 0x1B1B, 0x1B1B
};

// 2. Constructor
CIO_TYPE1::CIO_TYPE1() {
    _byte_count = 0;
    _bit_count = 0;
    _received_byte = 0;
    _send_bit = 8;
    _CIO_cmd_matches = 0;
    _packet_error = 0;
    _packet_transm_active = false;
    _data_is_output = false;
    _new_packet_available = false;
    _brightness = 7;
}

char CIO_TYPE1::_getChar(uint8_t value) {
    for (int i = 0; i < 38; i++) {
        if (CHARCODES[i] == value) return CHARS[i];
    }
    return '?';
}

uint16_t CIO_PRE2021::getButtonCode(Buttons button_index) {
    if (button_index >= 11) return _button_codes[0];
    return _button_codes[button_index];
}

void CIO_TYPE1::setup(int cio_data_pin, int cio_clk_pin, int cio_cs_pin) {
    _DATA_PIN = cio_data_pin;
    _CLK_PIN = cio_clk_pin;
    _CS_PIN = cio_cs_pin;
    pinMode(_DATA_PIN, INPUT);
    pinMode(_CLK_PIN, INPUT);
    pinMode(_CS_PIN, INPUT);
}

// 3. High-Speed Interrupt Handlers (ISRs)
void IRAM_ATTR CIO_TYPE1::isr_clkHandler() {
    if (!_packet_transm_active) return;
    
    uint32_t gpio_in = READ_PERI_REG(PIN_IN);
    bool clockstate = (gpio_in >> _CLK_PIN) & 1;

    // Handle Data Injection (Output)
    if (!clockstate && _data_is_output) {
        if (_button_code & (1 << _send_bit)) WRITE_PERI_REG(PIN_OUT_SET, 1 << _DATA_PIN);
        else WRITE_PERI_REG(PIN_OUT_CLEAR, 1 << _DATA_PIN);
        _send_bit++; 
        if (_send_bit > 15) _send_bit = 0;
    }

    // Handle Data Capture (Input)
    if (clockstate && !_data_is_output) {
        _received_byte = (_received_byte >> 1) | (((gpio_in >> _DATA_PIN) & 1) << 7);
        _bit_count++;
        
        if (_bit_count == 8) {
            if (_CIO_cmd_matches == 2 && _byte_count < 11) {
                _payload[_byte_count++] = _received_byte;
            } else if (_received_byte == 0x44) { // DSP_CMD2_DATAREAD
                _send_bit = 8; 
                _data_is_output = true;
                WRITE_PERI_REG(PIN_DIR_OUTPUT, 1 << _DATA_PIN);
            }
            _bit_count = 0;
        }
    }
}

void IRAM_ATTR CIO_TYPE1::isr_packetHandler() {
    if (READ_PERI_REG(PIN_IN) & (1 << _CS_PIN)) {
        _packet_transm_active = false; 
        _data_is_output = false; 
        eopHandler();
    } else { 
        _packet_transm_active = true; 
    }
}

void IRAM_ATTR CIO_TYPE1::eopHandler() {
    WRITE_PERI_REG(PIN_DIR_INPUT, 1 << _DATA_PIN);
    if (_byte_count != 11 && _byte_count != 0) _packet_error |= 2;
    if (_bit_count != 0) _packet_error |= 1;
    
    _byte_count = 0; 
    _bit_count = 0;
}

// 4. Main Loop Logic
void CIO_TYPE1::updateStates() {
    if (_packet_error) {
        bad_packets_count++;
        _packet_error = 0;
        return;
    }
    // State parsing logic from _payload goes here
    good_packets_count++;
}

} // namespace bestway_va
