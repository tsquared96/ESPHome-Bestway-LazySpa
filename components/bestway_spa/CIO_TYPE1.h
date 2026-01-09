#pragma once
#include <Arduino.h>
#include "enums.h"

// Low-level register access for ESP8266 high-speed timing
#define PIN_IN          0x60000318
#define PIN_OUT_SET     0x60000304
#define PIN_OUT_CLEAR   0x60000308
#define PIN_DIR_OUTPUT  0x60000300
#define PIN_DIR_INPUT   0x60000300

namespace bestway_va {

class CIO_TYPE1 {
public:
    CIO_TYPE1();
    void setup(int cio_data_pin, int cio_clk_pin, int cio_cs_pin);
    void updateStates();
    void setButtonCode(uint16_t code) { _button_code = code; }
    virtual uint16_t getButtonCode(Buttons button_index) = 0;
    
    // Missing constants the .cpp needs
    static const uint8_t CHARCODES[38];
    static const char CHARS[38];

    void IRAM_ATTR isr_packetHandler();
    void IRAM_ATTR isr_clkHandler();

    sStates cio_states;
    uint8_t brightness = 7;
    uint32_t good_packets_count = 0;
    uint32_t bad_packets_count = 0;

protected:
    char _getChar(uint8_t value);
    void IRAM_ATTR eopHandler();
    
    int _DATA_PIN;
    int _CLK_PIN;
    int _CS_PIN;

    // These were missing and causing your current errors:
    volatile uint8_t _byte_count;
    volatile uint8_t _bit_count;
    volatile uint8_t _received_byte;
    volatile uint8_t _payload[11];
    volatile uint8_t _send_bit;
    volatile uint16_t _button_code;
    volatile uint8_t _CIO_cmd_matches;
    volatile uint8_t _packet_error;
    volatile bool _packet_transm_active;
    volatile bool _data_is_output;
    volatile bool _new_packet_available;
    volatile uint8_t _brightness;
};

class CIO_PRE2021 : public CIO_TYPE1 {
public:
    uint16_t getButtonCode(Buttons button_index) override;
protected:
    static const uint16_t _button_codes[11];
};

} // namespace bestway_va
