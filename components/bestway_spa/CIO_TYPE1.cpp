#include "CIO_TYPE1.h"

namespace bestway_va {

// 1. Define the static lookup tables declared in the .h
const uint8_t CIO_TYPE1::CHARCODES[38] = {
    0x7E, 0x30, 0x6D, 0x79, 0x33, 0x5B, 0x5F, 0x70, 0x7F, 0x7B, // 0-9
    0x77, 0x1F, 0x4E, 0x3D, 0x4F, 0x47, 0x5E, 0x37, 0x06, 0x3C, // A-J
    0x57, 0x0E, 0x54, 0x15, 0x1D, 0x67, 0x73, 0x05, 0x5B, 0x0F, // K-T
    0x3E, 0x1C, 0x5C, 0x49, 0x3B, 0x25, 0x00, 0x40              // U-Z, Space, Dash
};

const char CIO_TYPE1::CHARS[38] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z', ' ', '-'
};

// Button codes for the Pre-2021 (Type 1) protocol
const uint16_t CIO_PRE2021::_button_codes[11] = {
    0x1B1B, // NOBTN
    0x1B11, // PUMP
    0x1B0B, // HEAT
    0x1B17, // BUBBLES
    0x1B1D, // LOCK
    0x1B1E, // TEMP_UP
    0x1B1D, // TEMP_DOWN (Same as lock on some models)
    0x0B1B, // POWER
    0x1B1B, // UNIT
    0x1B1B, // RESET
    0x1B1B  // TIMER
};

// 2. Constructor: Initialize all those volatile variables
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

// 3. Logic for translating the 7-segment display codes to characters
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

// 4. Setup: Attach the high-priority interrupts
void CIO_TYPE1::setup(int cio_data_pin, int cio_clk_pin, int cio_cs_pin) {
    _DATA_PIN = cio_data_pin;
    _CLK_PIN = cio_clk_pin;
    _CS_PIN = cio_cs_pin;

    pinMode(_DATA_PIN, INPUT);
    pinMode(_CLK_PIN, INPUT);
    pinMode(_CS_PIN, INPUT);

    // We use functional interrupts to point to our IRAM_ATTR handlers
    // Note: External glue code usually handles the actual attachInterrupt call 
    // to point to the global instance.
}

// 5. updateStates: This is called by the main loop to process the raw payload
void CIO_TYPE1::updateStates() {
    if (_packet_error) {
        bad_packets_count++;
        _packet_error = 0;
        return;
    }

    // Example mapping: Payload[1] is usually the first digit on the screen
    cio_states.temperature = (uint8_t)_getChar(_payload[1]); 
    // ... add further state parsing logic here ...
    
    good_packets_count++;
}

} // namespace bestway_va
