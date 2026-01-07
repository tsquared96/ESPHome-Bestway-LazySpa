/**
 * CIO TYPE2 Protocol Handler Implementation
 *
 * Adapted from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 * https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 * Original file: Code/lib/cio/CIO_TYPE2.cpp
 *
 * Original work Copyright (c) visualapproach
 * Licensed under GPL v3
 */

#include "CIO_TYPE2.h"
#include "ports.h"

namespace bestway_va {

CIO_TYPE2* g_cio_type2_instance = nullptr;

// 7-segment character codes for TYPE2
const uint8_t CIO_TYPE2::CHARCODES[38] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x00, 0x40, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71, 0x7D,
    0x74, 0x76, 0x30, 0x0E, 0x70, 0x38, 0x00, 0x54, 0x5C, 0x73, 0x67, 0x50, 0x6D, 0x78, 0x1C, 0x3E, 0x00, 0x6E, 0x5B
};

// Button codes for 54149E model
const uint16_t CIO_54149E::_button_codes[11] = {
    0,      // NOBTN
    1<<7,   // LOCK
    1<<6,   // TIMER
    1<<5,   // BUBBLES
    1<<4,   // UNIT
    1<<3,   // HEAT
    1<<2,   // PUMP
    1<<1,   // DOWN
    1<<0,   // UP
    1<<8,   // POWER
    1<<9    // HYDROJETS
};

// ISR wrapper functions
static void IRAM_ATTR isr_LEDdatapin() {
    if (g_cio_type2_instance) {
        g_cio_type2_instance->LED_Handler();
    }
}

static void IRAM_ATTR isr_clk_type2() {
    if (g_cio_type2_instance) {
        g_cio_type2_instance->clkHandler();
    }
}

CIO_TYPE2::CIO_TYPE2() {
    _byte_count = 0;
    _bit_count = 0;
    _received_byte = 0;
    _send_bit = 8;
    _brightness = 7;
    _received_cmd = 0;
    _new_packet_available = false;
    _packet_transm_active = false;
    _button_code = 0;
}

void CIO_TYPE2::setup(int cio_td_pin, int cio_clk_pin, int cio_ld_pin) {
    g_cio_type2_instance = this;
    _CIO_TD_PIN = cio_td_pin;
    _CIO_CLK_PIN = cio_clk_pin;
    _CIO_LD_PIN = cio_ld_pin;
    _button_code = getButtonCode(NOBTN);

    pinMode(_CIO_LD_PIN, INPUT);
    pinMode(_CIO_TD_PIN, OUTPUT);
    pinMode(_CIO_CLK_PIN, INPUT);
    digitalWrite(_CIO_TD_PIN, HIGH);  // Idle high

    attachInterrupt(digitalPinToInterrupt(_CIO_LD_PIN), isr_LEDdatapin, CHANGE);
    attachInterrupt(digitalPinToInterrupt(_CIO_CLK_PIN), isr_clk_type2, CHANGE);
}

void CIO_TYPE2::stop() {
    detachInterrupt(digitalPinToInterrupt(_CIO_LD_PIN));
    detachInterrupt(digitalPinToInterrupt(_CIO_CLK_PIN));
}

void CIO_TYPE2::pause_all(bool action) {
    if (action) {
        detachInterrupt(digitalPinToInterrupt(_CIO_LD_PIN));
        detachInterrupt(digitalPinToInterrupt(_CIO_CLK_PIN));
    } else {
        attachInterrupt(digitalPinToInterrupt(_CIO_LD_PIN), isr_LEDdatapin, CHANGE);
        attachInterrupt(digitalPinToInterrupt(_CIO_CLK_PIN), isr_clk_type2, CHANGE);
    }
}

void CIO_TYPE2::updateStates() {
    if (!_new_packet_available) return;
    _new_packet_available = false;

    static uint32_t buttonReleaseTime;
    enum Readmode : int { readtemperature, uncertain, readtarget };
    static Readmode capturePhase = readtemperature;

    // Copy payload to public array
    for (unsigned int i = 0; i < sizeof(_payload); i++) {
        _raw_payload_from_cio[i] = _payload[i];
    }

    good_packets_count++;
    brightness = _brightness & 7;

    // Parse states from payload
    cio_states.locked = (_raw_payload_from_cio[LCK_IDX] & (1 << LCK_BIT)) > 0;
    cio_states.power = 1;  // Always considered ON for TYPE2

    // Unit (C/F) - only update if one LED is on
    if (_raw_payload_from_cio[C_IDX] & (1 << C_BIT) || _raw_payload_from_cio[F_IDX] & (1 << F_BIT)) {
        cio_states.unit = (_raw_payload_from_cio[C_IDX] & (1 << C_BIT)) > 0;
    }

    cio_states.bubbles = (_raw_payload_from_cio[AIR_IDX] & (1 << AIR_BIT)) > 0;
    cio_states.heatgrn = (_raw_payload_from_cio[GRNHTR_IDX] & (1 << GRNHTR_BIT)) > 0;
    cio_states.heatred = (_raw_payload_from_cio[REDHTR_IDX] & (1 << REDHTR_BIT)) > 0;
    cio_states.timerled1 = (_raw_payload_from_cio[TMR1_IDX] & (1 << TMR1_BIT)) > 0;
    cio_states.timerled2 = (_raw_payload_from_cio[TMR2_IDX] & (1 << TMR2_BIT)) > 0;
    cio_states.timerbuttonled = (_raw_payload_from_cio[TMRBTNLED_IDX] & (1 << TMRBTNLED_BIT)) > 0;
    cio_states.heat = cio_states.heatgrn || cio_states.heatred;
    cio_states.pump = (_raw_payload_from_cio[FLT_IDX] & (1 << FLT_BIT)) > 0;

    // Decode display characters
    cio_states.char1 = (uint8_t)_getChar(_raw_payload_from_cio[DGT1_IDX]);
    cio_states.char2 = (uint8_t)_getChar(_raw_payload_from_cio[DGT2_IDX]);
    cio_states.char3 = (uint8_t)_getChar(_raw_payload_from_cio[DGT3_IDX]);

    if (getHasjets()) {
        cio_states.jets = (_raw_payload_from_cio[HJT_IDX] & (1 << HJT_BIT)) > 0;
    } else {
        cio_states.jets = 0;
    }

    // Check for unreadable characters
    if (cio_states.char1 == '*' || cio_states.char2 == '*' || cio_states.char3 == '*') return;

    // Check for error display
    if (cio_states.char1 == 'e') {
        String errornumber;
        errornumber = (char)cio_states.char2;
        errornumber += (char)cio_states.char3;
        cio_states.error = (uint8_t)(errornumber.toInt());
        return;
    }

    if (cio_states.char3 == 'H' || cio_states.char3 == ' ') return;

    // Reset error state
    cio_states.error = 0;

    // Capture TARGET after UP/DOWN pressed
    if ((_button_code == getButtonCode(UP)) || (_button_code == getButtonCode(DOWN))) {
        buttonReleaseTime = millis();
        if (cio_states.power && !cio_states.locked) capturePhase = readtarget;
    }

    // Stop expecting target temp after timeout
    if ((millis() - buttonReleaseTime) > 2000) capturePhase = uncertain;
    if ((millis() - buttonReleaseTime) > 6000) capturePhase = readtemperature;

    // Convert display to temperature value
    String tempstring = String((char)cio_states.char1) + String((char)cio_states.char2) + String((char)cio_states.char3);
    uint8_t parsedValue = tempstring.toInt();

    if ((capturePhase == readtarget) && (parsedValue > 19)) {
        cio_states.target = parsedValue;
    }

    if (capturePhase == readtemperature) {
        if (cio_states.temperature != parsedValue) {
            cio_states.temperature = parsedValue;
        }
    }
}

// Packet start/stop handler
void IRAM_ATTR CIO_TYPE2::LED_Handler() {
#ifdef ESP8266
    // Check START/END condition: LD_PIN change when CLK_PIN is high
    if (READ_PERI_REG(PIN_IN) & (1 << _CIO_CLK_PIN)) {
        _byte_count = 0;
        _bit_count = 0;
        _received_cmd = 0;
        _new_packet_available = (READ_PERI_REG(PIN_IN) & (1 << _CIO_LD_PIN)) > 0;
        _packet_transm_active = !_new_packet_available;
    }
#endif
}

void IRAM_ATTR CIO_TYPE2::clkHandler() {
#ifdef ESP8266
    uint16_t td_bitnumber = _bit_count % 10;
    uint16_t ld_bitnumber = _bit_count % 8;
    uint16_t buttonwrapper = (0xFE << 8) | (_button_code << 1);  // startbit @ bit0, stopbit @ bit9

    // Rising or falling edge?
    bool risingedge = READ_PERI_REG(PIN_IN) & (1 << _CIO_CLK_PIN);

    if (risingedge) {
        // Clock rising edge - read data
        _byte_count = _bit_count / 8;

        if (_byte_count == 0) {
            _received_cmd |= ((READ_PERI_REG(PIN_IN) & (1 << _CIO_LD_PIN)) > 0) << ld_bitnumber;
        } else if ((_byte_count < 6) && (_received_cmd == CMD2)) {
            // Write to payload after CMD2
            _payload[_byte_count - 1] = (_payload[_byte_count - 1] & ~(1 << ld_bitnumber)) |
                                        ((READ_PERI_REG(PIN_IN) & (1 << _CIO_LD_PIN)) > 0) << ld_bitnumber;
        }

        // Store brightness
        if (_bit_count == 7 && (_received_cmd & 0xC0) == 0x80) {
            _brightness = _received_cmd;
        }

        _bit_count++;
    } else {
        // Clock falling edge - write data
        if (buttonwrapper & (1 << td_bitnumber)) {
            WRITE_PERI_REG(PIN_OUT_SET, 1 << _CIO_TD_PIN);
        } else {
            WRITE_PERI_REG(PIN_OUT_CLEAR, 1 << _CIO_TD_PIN);
        }
    }
#endif
}

char CIO_TYPE2::_getChar(uint8_t value) {
    for (unsigned int index = 0; index < sizeof(CHARCODES); index++) {
        if (value == CHARCODES[index]) {
            return CHARS[index];
        }
    }
    return '*';
}

// 54149E button code implementation
uint16_t CIO_54149E::getButtonCode(Buttons button_index) {
    if (button_index < 11) {
        return _button_codes[button_index];
    }
    return 0;
}

Buttons CIO_54149E::getButton(uint16_t code) {
    for (int i = 0; i < 11; i++) {
        if (_button_codes[i] == code) {
            return (Buttons)i;
        }
    }
    return NOBTN;
}

}  // namespace bestway_va
