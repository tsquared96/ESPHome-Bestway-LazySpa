/**
 * CIO TYPE2 Protocol Handler
 *
 * Adapted from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 * https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 * Original file: Code/lib/cio/CIO_TYPE2.h
 *
 * For models: 54149E (6-wire TYPE2 protocol)
 *
 * TYPE2 differs from TYPE1:
 * - Different packet structure (5 bytes vs 11)
 * - Different LED handling
 * - Different button codes
 *
 * Original work Copyright (c) visualapproach
 * Licensed under GPL v3
 */

#pragma once
#include <Arduino.h>
#include "enums.h"

namespace bestway_va {

class CIO_TYPE2 {
public:
    CIO_TYPE2();

    void setup(int cio_td_pin, int cio_clk_pin, int cio_ld_pin);
    void stop();
    void pause_all(bool action);
    void updateStates();

    void setButtonCode(uint16_t code) { _button_code = code; }
    virtual uint16_t getButtonCode(Buttons button_index) = 0;
    virtual Buttons getButton(uint16_t code) = 0;
    virtual bool getHasjets() = 0;
    virtual bool getHasair() = 0;

    void IRAM_ATTR LED_Handler();
    void IRAM_ATTR clkHandler();

    sStates cio_states;
    uint8_t brightness = 7;
    uint32_t good_packets_count = 0;
    uint32_t bad_packets_count = 0;

    uint8_t _raw_payload_from_cio[5];

protected:
    char _getChar(uint8_t value);

    int _CIO_TD_PIN;
    int _CIO_CLK_PIN;
    int _CIO_LD_PIN;

    volatile int _byte_count = 0;
    volatile int _bit_count = 0;
    volatile int _send_bit = 8;
    volatile uint8_t _received_byte;
    volatile uint8_t _brightness;
    volatile uint8_t _payload[5];
    uint8_t _prev_payload[5];
    uint8_t _received_cmd;
    volatile bool _packet = false;
    volatile bool _new_packet_available;
    volatile bool _packet_transm_active;
    volatile uint16_t _button_code;

    // Clock pulse width
    static const uint16_t CLKPW = 50;

    // Command bytes
    static const uint8_t CMD1 = 0x40;  // Normal mode, auto+1 address
    static const uint8_t CMD2 = 0xC0;  // Start address 00H

    // Display brightness constants
    static const uint8_t DSP_DIM_BASE = 0x80;
    static const uint8_t DSP_DIM_ON = 0x08;

    // Payload byte indices and bit positions (LSB first)
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

    // 7-segment character codes (MSB-> .gfedcba <-LSB)
    static const uint8_t CHARCODES[38];

    // ESP8266 GPIO register addresses
#ifdef ESP8266
    static const uint32_t PIN_IN = 0x60000318;
    static const uint32_t PIN_OUT_SET = 0x60000304;
    static const uint32_t PIN_OUT_CLEAR = 0x60000308;
#endif
};

// 54149E model implementation
class CIO_54149E : public CIO_TYPE2 {
public:
    uint16_t getButtonCode(Buttons button_index) override;
    Buttons getButton(uint16_t code) override;
    bool getHasjets() override { return false; }
    bool getHasair() override { return true; }

private:
    static const uint16_t _button_codes[11];
};

extern CIO_TYPE2* g_cio_type2_instance;

}  // namespace bestway_va
