#include "DSP_TYPE1.h"

namespace bestway_va {

// Constructor
DSP_TYPE1::DSP_TYPE1() {
    _byte_count = 0;
    _bit_count = 0;
    _received_byte = 0;
    _packet_transm_active = false;
    _new_packet_available = false;
}

// Setup pins and state
void DSP_TYPE1::setup(int dsp_data_pin, int dsp_clk_pin, int dsp_cs_pin, int audio_pin) {
    _DATA_PIN = dsp_data_pin;
    _CLK_PIN = dsp_clk_pin;
    _CS_PIN = dsp_cs_pin;
    _AUDIO_PIN = audio_pin;

    pinMode(_DATA_PIN, INPUT);
    pinMode(_CLK_PIN, INPUT);
    pinMode(_CS_PIN, INPUT);
    if (_AUDIO_PIN != -1) {
        pinMode(_AUDIO_PIN, OUTPUT);
        digitalWrite(_AUDIO_PIN, LOW);
    }
}

// High-speed clock interrupt for the Display bus
void IRAM_ATTR DSP_TYPE1::isr_clkHandler() {
    if (!_packet_transm_active) return;

    uint32_t gpio_in = READ_PERI_REG(PIN_IN);
    bool clockstate = (gpio_in >> _CLK_PIN) & 1;

    // Capture data on the rising edge
    if (clockstate) {
        _received_byte = (_received_byte >> 1) | (((gpio_in >> _DATA_PIN) & 1) << 7);
        _bit_count++;

        if (_bit_count == 8) {
            if (_byte_count < 11) {
                _payload[_byte_count++] = _received_byte;
            }
            _bit_count = 0;
        }
    }
}

// Chip Select interrupt to mark start/end of display packets
void IRAM_ATTR DSP_TYPE1::isr_packetHandler() {
    if (READ_PERI_REG(PIN_IN) & (1 << _CS_PIN)) {
        _packet_transm_active = false;
        _new_packet_available = true;
    } else {
        _packet_transm_active = true;
        _byte_count = 0;
        _bit_count = 0;
    }
}

// Processes the raw bytes into a button press
void DSP_TYPE1::handleStates() {
    if (!_new_packet_available) return;
    _new_packet_available = false;

    // The button state is typically in the last few bytes of the 11-byte packet
    // This logic checks for specific bitmask changes
    uint8_t btn_low = _payload[8];
    uint8_t btn_high = _payload[9];

    // Simple mapping example:
    if (btn_low == 0x11) _last_btn = PUMP;
    else if (btn_low == 0x0B) _last_btn = HEAT;
    else if (btn_low == 0x17) _last_btn = BUBBLES;
    else if (btn_low == 0x1D) _last_btn = LOCK;
    else _last_btn = NOBTN;
}

Buttons DSP_TYPE1::getPressedButton() {
    Buttons tmp = _last_btn;
    // We don't clear it here so that the main loop can 
    // handle debouncing/long presses
    return tmp;
}

} // namespace bestway_va
