/**
 * CIO TYPE1 Protocol Handler
 *
 * Adapted from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 * https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 * Original file: Code/lib/cio/CIO_TYPE1.h
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

class CIO_TYPE1 {
public:
    CIO_TYPE1();

    // Setup pins and attach interrupts
    void setup(int cio_data_pin, int cio_clk_pin, int cio_cs_pin);

    // Detach interrupts
    void stop();

    // Pause/resume interrupts
    void pause_all(bool action);

    // Process received packets and update states
    void updateStates();

    // Set button code to transmit
    void setButtonCode(uint16_t code) { _button_code = code; }

    // Get button code for a button index (model-specific)
    virtual uint16_t getButtonCode(Buttons button_index) = 0;

    // Check model capabilities
    virtual bool getHasjets() = 0;
    virtual bool getHasair() = 0;

    // ISR handlers (called by static wrappers)
    void IRAM_ATTR isr_packetHandler();
    void IRAM_ATTR isr_clkHandler();

    // Public state
    sStates cio_states;
    uint8_t brightness = 7;
    uint32_t good_packets_count = 0;
    uint32_t bad_packets_count = 0;
    uint8_t packet_error = 0;

    // Debug counters for button transmission
    volatile uint32_t cmd_read_count = 0;      // Times 0x42 (button read) received
    volatile uint32_t button_bits_sent = 0;    // Button bits transmitted
    volatile uint16_t last_btn_transmitted = 0; // Last button code we tried to send
    volatile uint32_t btn_press_count = 0;     // Times we transmitted a non-NOBTN button

    // Raw payload for DSP forwarding
    uint8_t _raw_payload_from_cio[11];

protected:
    void IRAM_ATTR eopHandler();
    char _getChar(uint8_t value);

    // Pin numbers
    int _DATA_PIN;
    int _CLK_PIN;
    int _CS_PIN;

    // Protocol state
    volatile int _byte_count;
    volatile int _bit_count;
    volatile int _CIO_cmd_matches;
    volatile int _send_bit;
    volatile uint8_t _received_byte;
    volatile uint8_t _brightness;
    volatile uint8_t _payload[11];
    volatile uint8_t _packet_error;
    volatile bool _data_is_output;
    volatile bool _new_packet_available;
    volatile bool _packet_transm_active;
    volatile uint16_t _button_code;

    // Command constants
    static const uint8_t DSP_CMD1_MODE6_11_7 = 0x01;
    static const uint8_t DSP_CMD1_MODE6_11_7_P05504 = 0x05;
    static const uint8_t DSP_CMD2_DATAREAD = 0x42;
    static const uint8_t DSP_CMD2_DATAWRITE = 0x40;

    // Payload byte indices and bit positions
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

    // ESP8266 GPIO register addresses
#ifdef ESP8266
    static const uint32_t PIN_IN = 0x60000318;
    static const uint32_t PIN_OUT_SET = 0x60000304;
    static const uint32_t PIN_OUT_CLEAR = 0x60000308;
    static const uint32_t PIN_DIR_INPUT = 0x60000314;
    static const uint32_t PIN_DIR_OUTPUT = 0x60000310;
#endif
};

// PRE2021 model implementation
class CIO_PRE2021 : public CIO_TYPE1 {
public:
    uint16_t getButtonCode(Buttons button_index) override;
    bool getHasjets() override { return false; }
    bool getHasair() override { return true; }

private:
    static const uint16_t _button_codes[11];
};

// Global instance pointer for ISR access
extern CIO_TYPE1* g_cio_instance;

}  // namespace bestway_va
