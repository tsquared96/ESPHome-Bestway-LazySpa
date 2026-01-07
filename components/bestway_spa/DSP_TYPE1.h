/**
 * DSP TYPE1 Protocol Handler
 *
 * Adapted from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 * https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 * Original file: Code/lib/dsp/DSP_TYPE1.h
 *
 * For models: PRE2021, P05504 (6-wire SPI-like protocol)
 *
 * Original work Copyright (c) visualapproach
 * Licensed under GPL v3
 */

#pragma once
#include <Arduino.h>
#include "enums.h"

namespace bestway_va {

class DSP_TYPE1 {
public:
    DSP_TYPE1();

    // Setup pins
    void setup(int dsp_data_pin, int dsp_clk_pin, int dsp_cs_pin, int dsp_audio_pin);

    // Stop (disable audio)
    void stop();

    // Update display with current state (call from main loop)
    void handleStates();

    // Read button press from physical display
    Buttons getPressedButton();

    // Convert button code to button index
    virtual Buttons buttonCodeToIndex(uint16_t code) = 0;

    // Public state (copied from CIO)
    sStates dsp_states;

    // Custom text to display (overrides state chars)
    String text;

    // Audio frequency (0 = off)
    uint16_t audiofrequency = 0;

    // Statistics
    uint32_t good_packets_count = 0;

    // Raw payload from DSP button reads
    uint8_t _raw_payload_from_dsp[2];

protected:
    void uploadPayload(uint8_t brightness);
    uint8_t charTo7SegmCode(char c);
    void _sendBitsToDSP(uint32_t out_bits, int bit_count);
    uint16_t _receiveBitsFromDSP();
    void clearpayload();

    // Pin numbers
    int _DATA_PIN;
    int _CLK_PIN;
    int _CS_PIN;
    int _AUDIO_PIN;

    // Timing
    unsigned long _dsp_last_refreshtime = 0;
    unsigned long _dsp_getbutton_last_time = 0;

    // Payload buffer
    uint8_t _payload[11] = {0xC0, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00};

    // Last button state
    Buttons _old_button = NOBTN;

    // Command constants
    static const uint8_t DSP_DIM_BASE = 0x80;
    static const uint8_t DSP_DIM_ON = 0x08;
    static const uint8_t DSP_CMD1_MODE6_11_7 = 0x01;
    static const uint8_t DSP_CMD1_MODE6_11_7_P05504 = 0x05;
    static const uint8_t DSP_CMD2_DATAREAD = 0x42;
    static const uint8_t DSP_CMD2_DATAWRITE = 0x40;

    // Payload byte indices
    static const uint8_t DGT1_IDX = 1;
    static const uint8_t DGT2_IDX = 3;
    static const uint8_t DGT3_IDX = 5;
    static const uint8_t TMR2_IDX = 7;
    static const uint8_t TMR2_BIT = 1;
    static const uint8_t TMR1_IDX = 7;
    static const uint8_t TMR1_BIT = 2;
    static const uint8_t LCK_IDX = 7;
    static const uint8_t LCK_BIT = 3;
    static const uint8_t TMRBTNLED_IDX = 7;
    static const uint8_t TMRBTNLED_BIT = 4;
    static const uint8_t REDHTR_IDX = 7;
    static const uint8_t REDHTR_BIT = 5;
    static const uint8_t GRNHTR_IDX = 7;
    static const uint8_t GRNHTR_BIT = 6;
    static const uint8_t AIR_IDX = 7;
    static const uint8_t AIR_BIT = 7;
    static const uint8_t FLT_IDX = 9;
    static const uint8_t FLT_BIT = 1;
    static const uint8_t C_IDX = 9;
    static const uint8_t C_BIT = 2;
    static const uint8_t F_IDX = 9;
    static const uint8_t F_BIT = 3;
    static const uint8_t PWR_IDX = 9;
    static const uint8_t PWR_BIT = 4;
    static const uint8_t HJT_IDX = 9;
    static const uint8_t HJT_BIT = 5;

    // 7-segment character codes
    static const uint8_t CHARCODES[38];
    static const char CHARS[38];
};

// PRE2021 model implementation
class DSP_PRE2021 : public DSP_TYPE1 {
public:
    Buttons buttonCodeToIndex(uint16_t code) override;

private:
    static const uint16_t _button_codes[11];
};

}  // namespace bestway_va
