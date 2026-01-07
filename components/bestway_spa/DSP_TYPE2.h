/**
 * DSP TYPE2 Protocol Handler
 *
 * Adapted from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 * https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 * Original file: Code/lib/dsp/DSP_TYPE2.h
 *
 * For models: 54149E (6-wire TYPE2 protocol)
 *
 * DSP handles output to physical display panel and button reading
 *
 * Original work Copyright (c) visualapproach
 * Licensed under GPL v3
 */

#pragma once
#include <Arduino.h>
#include "enums.h"

namespace bestway_va {

class DSP_TYPE2 {
public:
    DSP_TYPE2();
    virtual ~DSP_TYPE2() {}

    void setup(int dsp_td_pin, int dsp_clk_pin, int dsp_ld_pin, int dsp_audio_pin);
    void stop();
    void handleStates();
    Buttons getPressedButton();

    virtual Buttons buttonCodeToIndex(uint16_t code) = 0;
    virtual bool getHasjets() = 0;

    sStates dsp_states;
    sToggles dsp_toggles;
    String text = "";
    int audiofrequency = 0;
    uint32_t good_packets_count = 0;
    bool EnabledButtons[11] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    uint8_t _raw_payload_from_dsp[2];

protected:
    void uploadPayload(uint8_t brightness);
    uint8_t charTo7SegmCode(char c);
    void clearpayload();
    void _sendBitsToDSP(uint32_t out_bits, int bit_count);

    int _DSP_TD_PIN;
    int _DSP_CLK_PIN;
    int _DSP_LD_PIN;
    int _DSP_AUDIO_PIN;

    unsigned long _dsp_last_refreshtime = 0;
    unsigned long _dsp_getbutton_last_time = 0;

    Buttons _old_button = NOBTN;
    Buttons _prev_button = NOBTN;
    uint8_t _payload[5];

    // Clock pulse width
    static const uint16_t CLKPW = 50;
    static const uint8_t DSP_DIM_BASE = 0x80;
    static const uint8_t DSP_DIM_ON = 0x08;

    // Command bytes (LSB)
    static const uint8_t CMD1 = 0x40;  // Normal mode, auto+1 address
    static const uint8_t CMD2 = 0xC0;  // Start address 00H
    static const uint8_t CMD3 = DSP_DIM_BASE | DSP_DIM_ON | 7;  // Full brightness

    // Payload byte indices and bit positions
    static const uint8_t DGT1_IDX = 0;
    static const uint8_t DGT2_IDX = 1;
    static const uint8_t DGT3_IDX = 2;
    static const uint8_t TMR2_IDX = 3;
    static const uint8_t TMR2_BIT = 7;
    static const uint8_t TMR1_IDX = 3;
    static const uint8_t TMR1_BIT = 6;
    static const uint8_t LCK_IDX = 3;
    static const uint8_t LCK_BIT = 5;
    static const uint8_t TMRBTNLED_IDX = 3;
    static const uint8_t TMRBTNLED_BIT = 4;
    static const uint8_t REDHTR_IDX = 3;
    static const uint8_t REDHTR_BIT = 2;
    static const uint8_t GRNHTR_IDX = 3;
    static const uint8_t GRNHTR_BIT = 3;
    static const uint8_t AIR_IDX = 3;
    static const uint8_t AIR_BIT = 1;
    static const uint8_t FLT_IDX = 4;
    static const uint8_t FLT_BIT = 2;
    static const uint8_t C_IDX = 4;
    static const uint8_t C_BIT = 0;
    static const uint8_t F_IDX = 4;
    static const uint8_t F_BIT = 1;
    static const uint8_t PWR_IDX = 4;
    static const uint8_t PWR_BIT = 3;
    static const uint8_t HJT_IDX = 4;
    static const uint8_t HJT_BIT = 4;

    // 7-segment character codes
    static const uint8_t CHARCODES[38];
};

// 54149E model implementation
class DSP_54149E : public DSP_TYPE2 {
public:
    Buttons buttonCodeToIndex(uint16_t code) override;
    bool getHasjets() override { return false; }

private:
    static const uint16_t _button_codes[11];
};

extern DSP_TYPE2* g_dsp_type2_instance;

}  // namespace bestway_va
