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
 *
 * TODO: Full implementation - currently placeholder
 */

#pragma once
#include <Arduino.h>
#include "enums.h"

namespace bestway_va {

class CIO_TYPE2 {
public:
    CIO_TYPE2();

    void setup(int cio_data_pin, int cio_clk_pin, int cio_cs_pin);
    void stop();
    void pause_all(bool action);
    void updateStates();

    void setButtonCode(uint16_t code) { _button_code = code; }
    virtual uint16_t getButtonCode(Buttons button_index) = 0;
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

    int _DATA_PIN;
    int _CLK_PIN;
    int _CS_PIN;  // Called LD (load) in TYPE2

    volatile int _byte_count;
    volatile int _bit_count;
    volatile uint8_t _received_byte;
    volatile uint8_t _payload[5];
    volatile bool _new_packet_available;
    volatile uint16_t _button_code;

    // TYPE2 uses different byte positions
    // Payload structure (5 bytes):
    // Byte 0: Digit 1 (7-seg)
    // Byte 1: Digit 2 (7-seg)
    // Byte 2: Digit 3 (7-seg)
    // Byte 3: LEDs/status
    // Byte 4: More LEDs/status

    static const uint8_t CHARCODES[38];
    static const char CHARS[38];
};

// 54149E model implementation
class CIO_54149E : public CIO_TYPE2 {
public:
    uint16_t getButtonCode(Buttons button_index) override;
    bool getHasjets() override { return true; }
    bool getHasair() override { return true; }

private:
    static const uint16_t _button_codes[11];
};

extern CIO_TYPE2* g_cio_type2_instance;

}  // namespace bestway_va
