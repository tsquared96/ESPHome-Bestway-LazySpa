/**
 * DSP TYPE1 Protocol Handler
 *
 * Adapted from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 * https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 * Original file: Code/lib/dsp/DSP_TYPE1.cpp
 *
 * Original work Copyright (c) visualapproach
 * Licensed under GPL v3
 */

#include "DSP_TYPE1.h"

namespace bestway_va {

// 7-segment character codes
const uint8_t DSP_TYPE1::CHARCODES[38] = {
    0x7F, 0x0D, 0xB7, 0x9F, 0xCD, 0xDB, 0xFB, 0x0F, 0xFF, 0xDF,  // 0-9
    0x01, 0x81,  // space, dash
    0xEF, 0xF9, 0x73, 0xBD, 0xF3, 0xE3, 0xFB, 0xE9, 0xED, 0x61,  // A-K
    0x1D, 0xE1, 0x71, 0x01, 0xA9, 0xB9, 0xE7, 0xCF, 0xA1, 0xDB,  // L-U
    0xF1, 0x39, 0x7D, 0x01, 0xDD, 0xB7  // V-Z
};

const char DSP_TYPE1::CHARS[38] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ' ', '-',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

// PRE2021 button codes (from physical display)
const uint16_t DSP_PRE2021::_button_codes[11] = {
    0xFFFF,  // NOBTN
    0x0000,  // LOCK
    0x0001,  // TIMER
    0x0002,  // BUBBLES
    0x0004,  // UNIT
    0x0008,  // HEAT
    0x0010,  // PUMP
    0x0020,  // DOWN
    0x0080,  // UP
    0x8000,  // POWER
    0x0000   // HYDROJETS (not available)
};

Buttons DSP_PRE2021::buttonCodeToIndex(uint16_t code) {
    for (unsigned int i = 0; i < sizeof(_button_codes) / sizeof(uint16_t); i++) {
        if (code == _button_codes[i]) {
            return (Buttons)i;
        }
    }
    return NOBTN;
}

DSP_TYPE1::DSP_TYPE1() {
    // Default payload initialization per VA
    _payload[0] = 0xC0;
    _payload[1] = 0x01;
    _payload[2] = 0x00;
    _payload[3] = 0x01;
    _payload[4] = 0x00;
    _payload[5] = 0x01;
    _payload[6] = 0x00;
    _payload[7] = 0x01;
    _payload[8] = 0x00;
    _payload[9] = 0x01;
    _payload[10] = 0x00;
}

void DSP_TYPE1::setup(int dsp_data_pin, int dsp_clk_pin, int dsp_cs_pin, int dsp_audio_pin) {
    _DATA_PIN = dsp_data_pin;
    _CLK_PIN = dsp_clk_pin;
    _CS_PIN = dsp_cs_pin;
    _AUDIO_PIN = dsp_audio_pin;

    pinMode(_CS_PIN, OUTPUT);
    pinMode(_DATA_PIN, INPUT);
    pinMode(_CLK_PIN, OUTPUT);
    if (_AUDIO_PIN >= 0) {
        pinMode(_AUDIO_PIN, OUTPUT);
        digitalWrite(_AUDIO_PIN, LOW);
    }

    // CS idle high, CLK idle high
    digitalWrite(_CS_PIN, HIGH);
    digitalWrite(_CLK_PIN, HIGH);
}

void DSP_TYPE1::stop() {
    if (_AUDIO_PIN >= 0) {
        noTone(_AUDIO_PIN);
    }
}

uint8_t DSP_TYPE1::charTo7SegmCode(char c) {
    for (unsigned int index = 0; index < sizeof(CHARS); index++) {
        if (c == CHARS[index]) {
            return CHARCODES[index];
        }
    }
    return 0x00;  // space
}

void DSP_TYPE1::_sendBitsToDSP(uint32_t outBits, int bitsToSend) {
    pinMode(_DATA_PIN, OUTPUT);
    delayMicroseconds(20);
    for (int i = 0; i < bitsToSend; i++) {
        digitalWrite(_CLK_PIN, LOW);
        digitalWrite(_DATA_PIN, outBits & (1 << i));
        delayMicroseconds(20);
        digitalWrite(_CLK_PIN, HIGH);
        delayMicroseconds(20);
    }
}

uint16_t DSP_TYPE1::_receiveBitsFromDSP() {
    uint16_t result = 0;
    pinMode(_DATA_PIN, INPUT);

    for (int i = 0; i < 16; i++) {
        digitalWrite(_CLK_PIN, LOW);
        delayMicroseconds(20);
        digitalWrite(_CLK_PIN, HIGH);
        delayMicroseconds(20);
        int j = (i + 8) % 16;  // bit 8-15 then 0-7
        result |= digitalRead(_DATA_PIN) << j;
    }
    return result;
}

void DSP_TYPE1::clearpayload() {
    for (unsigned int i = 1; i < sizeof(_payload); i++) {
        _payload[i] = 0;
    }
}

Buttons DSP_TYPE1::getPressedButton() {
    if (millis() - _dsp_getbutton_last_time < 90) {
        return _old_button;
    }

    _dsp_getbutton_last_time = millis();

    // Send button read request
    digitalWrite(_CS_PIN, LOW);
    delayMicroseconds(50);
    _sendBitsToDSP(DSP_CMD2_DATAREAD, 8);
    uint16_t newButtonCode = _receiveBitsFromDSP();
    digitalWrite(_CS_PIN, HIGH);
    delayMicroseconds(30);

    if (newButtonCode != 0xFFFF) good_packets_count++;

    Buttons newButton = buttonCodeToIndex(newButtonCode);
    _old_button = newButton;
    _raw_payload_from_dsp[0] = newButtonCode >> 8;
    _raw_payload_from_dsp[1] = newButtonCode & 0xFF;

    return newButton;
}

void DSP_TYPE1::handleStates() {
    // Set display digits
    if (text.length()) {
        _payload[DGT1_IDX] = charTo7SegmCode(text[0]);
        _payload[DGT2_IDX] = text.length() > 1 ? charTo7SegmCode(text[1]) : 0x01;
        _payload[DGT3_IDX] = text.length() > 2 ? charTo7SegmCode(text[2]) : 0x01;
    } else {
        _payload[DGT1_IDX] = charTo7SegmCode(dsp_states.char1);
        _payload[DGT2_IDX] = charTo7SegmCode(dsp_states.char2);
        _payload[DGT3_IDX] = charTo7SegmCode(dsp_states.char3);
    }

    if (dsp_states.power) {
        // Status byte 7 - lock, timer, heater, bubbles
        _payload[LCK_IDX] &= ~(1 << LCK_BIT);
        _payload[LCK_IDX] |= dsp_states.locked << LCK_BIT;

        _payload[TMRBTNLED_IDX] &= ~(1 << TMRBTNLED_BIT);
        _payload[TMRBTNLED_IDX] |= dsp_states.timerbuttonled << TMRBTNLED_BIT;

        _payload[TMR1_IDX] &= ~(1 << TMR1_BIT);
        _payload[TMR1_IDX] |= dsp_states.timerled1 << TMR1_BIT;

        _payload[TMR2_IDX] &= ~(1 << TMR2_BIT);
        _payload[TMR2_IDX] |= dsp_states.timerled2 << TMR2_BIT;

        _payload[REDHTR_IDX] &= ~(1 << REDHTR_BIT);
        _payload[REDHTR_IDX] |= dsp_states.heatred << REDHTR_BIT;

        _payload[GRNHTR_IDX] &= ~(1 << GRNHTR_BIT);
        _payload[GRNHTR_IDX] |= dsp_states.heatgrn << GRNHTR_BIT;

        _payload[AIR_IDX] &= ~(1 << AIR_BIT);
        _payload[AIR_IDX] |= dsp_states.bubbles << AIR_BIT;

        // Status byte 9 - pump, unit, power, jets
        _payload[FLT_IDX] &= ~(1 << FLT_BIT);
        _payload[FLT_IDX] |= dsp_states.pump << FLT_BIT;

        _payload[C_IDX] &= ~(1 << C_BIT);
        _payload[C_IDX] |= (dsp_states.unit == 0) << C_BIT;

        _payload[F_IDX] &= ~(1 << F_BIT);
        _payload[F_IDX] |= (dsp_states.unit == 1) << F_BIT;

        _payload[PWR_IDX] &= ~(1 << PWR_BIT);
        _payload[PWR_IDX] |= dsp_states.power << PWR_BIT;

        _payload[HJT_IDX] &= ~(1 << HJT_BIT);
        _payload[HJT_IDX] |= dsp_states.jets << HJT_BIT;
    } else {
        clearpayload();
    }

    // Audio output
    if (_AUDIO_PIN >= 0) {
        if (audiofrequency) {
            tone(_AUDIO_PIN, audiofrequency);
        } else {
            noTone(_AUDIO_PIN);
        }
    }

    uploadPayload(dsp_states.brightness);
}

void DSP_TYPE1::uploadPayload(uint8_t brightness) {
    // Refresh at ~20Hz
    if (millis() - _dsp_last_refreshtime < 90) return;
    _dsp_last_refreshtime = millis();

    uint8_t enableLED = 0;
    if (brightness > 0) {
        enableLED = DSP_DIM_ON;
        brightness -= 1;
    }

    // Packet 1: Mode command
    delayMicroseconds(30);
    digitalWrite(_CS_PIN, LOW);
    _sendBitsToDSP(DSP_CMD1_MODE6_11_7_P05504, 8);
    digitalWrite(_CS_PIN, HIGH);

    // Packet 2: Data write command
    delayMicroseconds(50);
    digitalWrite(_CS_PIN, LOW);
    _sendBitsToDSP(DSP_CMD2_DATAWRITE, 8);
    digitalWrite(_CS_PIN, HIGH);

    // Packet 3: 11 payload bytes
    delayMicroseconds(50);
    digitalWrite(_CS_PIN, LOW);
    for (int i = 0; i < 11; i++) {
        _sendBitsToDSP(_payload[i], 8);
    }
    digitalWrite(_CS_PIN, HIGH);

    // Packet 4: Brightness
    delayMicroseconds(50);
    digitalWrite(_CS_PIN, LOW);
    _sendBitsToDSP(DSP_DIM_BASE | enableLED | brightness, 8);
    digitalWrite(_CS_PIN, HIGH);
    delayMicroseconds(50);
}

}  // namespace bestway_va
