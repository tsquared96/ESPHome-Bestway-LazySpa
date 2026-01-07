/**
 * DSP TYPE2 Protocol Handler Implementation
 *
 * Adapted from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 * https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 * Original file: Code/lib/dsp/DSP_TYPE2.cpp
 *
 * Original work Copyright (c) visualapproach
 * Licensed under GPL v3
 */

#include "DSP_TYPE2.h"

namespace bestway_va {

DSP_TYPE2* g_dsp_type2_instance = nullptr;

// 7-segment character codes for TYPE2
const uint8_t DSP_TYPE2::CHARCODES[38] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x00, 0x40, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71, 0x7D,
    0x74, 0x76, 0x30, 0x0E, 0x70, 0x38, 0x00, 0x54, 0x5C, 0x73, 0x67, 0x50, 0x6D, 0x78, 0x1C, 0x3E, 0x00, 0x6E, 0x5B
};

// Button codes for 54149E model
const uint16_t DSP_54149E::_button_codes[11] = {
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

DSP_TYPE2::DSP_TYPE2() {
    for (int i = 0; i < 5; i++) {
        _payload[i] = 0;
    }
}

void DSP_TYPE2::setup(int dsp_td_pin, int dsp_clk_pin, int dsp_ld_pin, int dsp_audio_pin) {
    g_dsp_type2_instance = this;
    _DSP_TD_PIN = dsp_td_pin;
    _DSP_CLK_PIN = dsp_clk_pin;
    _DSP_LD_PIN = dsp_ld_pin;
    _DSP_AUDIO_PIN = dsp_audio_pin;

    pinMode(_DSP_LD_PIN, OUTPUT);
    pinMode(_DSP_TD_PIN, INPUT);
    pinMode(_DSP_CLK_PIN, OUTPUT);
    pinMode(_DSP_AUDIO_PIN, OUTPUT);

    digitalWrite(_DSP_LD_PIN, HIGH);   // Idle high
    digitalWrite(_DSP_CLK_PIN, HIGH);  // Shift on falling, latch on rising
    digitalWrite(_DSP_AUDIO_PIN, LOW);
}

void DSP_TYPE2::stop() {
    noTone(_DSP_AUDIO_PIN);
}

uint8_t DSP_TYPE2::charTo7SegmCode(char c) {
    for (unsigned int index = 0; index < sizeof(CHARS); index++) {
        if (c == CHARS[index]) {
            return CHARCODES[index];
        }
    }
    return 0x00;  // No match, return 'space'
}

void DSP_TYPE2::_sendBitsToDSP(uint32_t outBits, int bitsToSend) {
    for (int i = 0; i < bitsToSend; i++) {
        digitalWrite(_DSP_CLK_PIN, LOW);
        delayMicroseconds(5);
        digitalWrite(_DSP_LD_PIN, outBits & (1 << i));
        delayMicroseconds(CLKPW - 5);
        digitalWrite(_DSP_CLK_PIN, HIGH);
        delayMicroseconds(CLKPW);
    }
}

void DSP_TYPE2::clearpayload() {
    for (unsigned int i = 0; i < sizeof(_payload); i++) {
        _payload[i] = 0;
    }
}

Buttons DSP_TYPE2::getPressedButton() {
    if (millis() - _dsp_getbutton_last_time < 20) return _old_button;

    uint16_t newButtonCode = 0;
    Buttons newButton;

    _dsp_getbutton_last_time = millis();

    // Startbit
    digitalWrite(_DSP_CLK_PIN, LOW);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_CLK_PIN, HIGH);
    delayMicroseconds(CLKPW);

    // Clock in 8 data bits
    for (int i = 0; i < 8; i++) {
        digitalWrite(_DSP_CLK_PIN, LOW);
        delayMicroseconds(CLKPW);
        digitalWrite(_DSP_CLK_PIN, HIGH);
        newButtonCode |= digitalRead(_DSP_TD_PIN) << i;
        delayMicroseconds(CLKPW);
    }

    // Stop bit
    digitalWrite(_DSP_CLK_PIN, LOW);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_CLK_PIN, HIGH);

    if (newButtonCode != 0xFFFF) good_packets_count++;

    newButton = buttonCodeToIndex(newButtonCode);
    _raw_payload_from_dsp[0] = newButtonCode >> 8;
    _raw_payload_from_dsp[1] = newButtonCode & 0xFF;

    // Debounce: only register change after two consecutive equal values
    if (newButton == _prev_button) {
        _old_button = newButton;
    } else {
        _prev_button = newButton;
    }

    return _old_button;
}

void DSP_TYPE2::handleStates() {
    // Convert characters to 7-segment codes
    if (text.length()) {
        _payload[DGT1_IDX] = charTo7SegmCode(text[0]);
        _payload[DGT2_IDX] = text.length() > 1 ? charTo7SegmCode(text[1]) : 1;
        _payload[DGT3_IDX] = text.length() > 2 ? charTo7SegmCode(text[2]) : 1;
    } else {
        _payload[DGT1_IDX] = charTo7SegmCode(dsp_states.char1);
        _payload[DGT2_IDX] = charTo7SegmCode(dsp_states.char2);
        _payload[DGT3_IDX] = charTo7SegmCode(dsp_states.char3);
    }

    if (dsp_states.power) {
        // Lock LED
        _payload[LCK_IDX] &= ~(1 << LCK_BIT);
        _payload[LCK_IDX] |= dsp_states.locked << LCK_BIT;

        // Timer button LED
        _payload[TMRBTNLED_IDX] &= ~(1 << TMRBTNLED_BIT);
        _payload[TMRBTNLED_IDX] |= dsp_states.timerbuttonled << TMRBTNLED_BIT;

        // Timer LEDs
        _payload[TMR1_IDX] &= ~(1 << TMR1_BIT);
        _payload[TMR1_IDX] |= dsp_states.timerled1 << TMR1_BIT;
        _payload[TMR2_IDX] &= ~(1 << TMR2_BIT);
        _payload[TMR2_IDX] |= dsp_states.timerled2 << TMR2_BIT;

        // Heater LEDs
        _payload[REDHTR_IDX] &= ~(1 << REDHTR_BIT);
        _payload[REDHTR_IDX] |= dsp_states.heatred << REDHTR_BIT;
        _payload[GRNHTR_IDX] &= ~(1 << GRNHTR_BIT);
        _payload[GRNHTR_IDX] |= dsp_states.heatgrn << GRNHTR_BIT;

        // Bubbles LED
        _payload[AIR_IDX] &= ~(1 << AIR_BIT);
        _payload[AIR_IDX] |= dsp_states.bubbles << AIR_BIT;

        // Filter pump LED
        _payload[FLT_IDX] &= ~(1 << FLT_BIT);
        _payload[FLT_IDX] |= dsp_states.pump << FLT_BIT;

        // Unit LEDs (C/F)
        _payload[C_IDX] &= ~(1 << C_BIT);
        _payload[C_IDX] |= dsp_states.unit << C_BIT;
        _payload[F_IDX] &= ~(1 << F_BIT);
        _payload[F_IDX] |= !dsp_states.unit << F_BIT;

        // Power LED
        _payload[PWR_IDX] &= ~(1 << PWR_BIT);
        _payload[PWR_IDX] |= dsp_states.power << PWR_BIT;

        // Jets LED
        _payload[HJT_IDX] &= ~(1 << HJT_BIT);
        _payload[HJT_IDX] |= dsp_states.jets << HJT_BIT;
    } else {
        clearpayload();
    }

    // Handle audio
    if (audiofrequency) {
        tone(_DSP_AUDIO_PIN, audiofrequency);
    } else {
        noTone(_DSP_AUDIO_PIN);
    }

    uploadPayload(dsp_states.brightness);
}

void DSP_TYPE2::uploadPayload(uint8_t brightness) {
    // Refresh display at ~10Hz
    if (millis() - _dsp_last_refreshtime < 100) return;
    _dsp_last_refreshtime = millis();

    uint8_t enableLED = 0;
    if (brightness > 0) {
        enableLED = DSP_DIM_ON;
        brightness -= 1;
    }

    // Packet 1: CMD1
    digitalWrite(_DSP_LD_PIN, LOW);
    delayMicroseconds(CLKPW);
    _sendBitsToDSP(CMD1, 8);
    digitalWrite(_DSP_CLK_PIN, LOW);
    digitalWrite(_DSP_LD_PIN, LOW);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_CLK_PIN, HIGH);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_LD_PIN, HIGH);
    delayMicroseconds(CLKPW);

    // Packet 2: CMD2 + data
    digitalWrite(_DSP_LD_PIN, LOW);
    delayMicroseconds(CLKPW);
    _sendBitsToDSP(CMD2, 8);
    for (unsigned int i = 0; i < sizeof(_payload); i++) {
        _sendBitsToDSP(_payload[i], 8);
    }
    digitalWrite(_DSP_CLK_PIN, LOW);
    digitalWrite(_DSP_LD_PIN, LOW);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_CLK_PIN, HIGH);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_LD_PIN, HIGH);
    delayMicroseconds(CLKPW);

    // Packet 3: CMD3 (brightness)
    digitalWrite(_DSP_LD_PIN, LOW);
    delayMicroseconds(CLKPW);
    _sendBitsToDSP((CMD3 & 0xF8) | enableLED | brightness, 8);
    digitalWrite(_DSP_CLK_PIN, LOW);
    digitalWrite(_DSP_LD_PIN, LOW);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_CLK_PIN, HIGH);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_LD_PIN, HIGH);
    delayMicroseconds(CLKPW);
}

// 54149E button code to index
Buttons DSP_54149E::buttonCodeToIndex(uint16_t code) {
    for (int i = 0; i < 11; i++) {
        if (_button_codes[i] == code) {
            return (Buttons)i;
        }
    }
    return NOBTN;
}

}  // namespace bestway_va
