#pragma once
#include <Arduino.h>
#include "enums.h"

namespace bestway_va {

class CIO_TYPE1 {
public:
    CIO_TYPE1();
    void setup(int cio_data_pin, int cio_clk_pin, int cio_cs_pin);
    void stop();
    void updateStates();
    void setButtonCode(uint16_t code) { _button_code = code; }
    virtual uint16_t getButtonCode(Buttons button_index) = 0;
    virtual bool getHasjets() = 0;
    virtual bool getHasair() = 0;

    void IRAM_ATTR isr_packetHandler();
    void IRAM_ATTR isr_clkHandler();

    sStates cio_states;
    uint8_t brightness = 7; // Fixed: Now public
    uint32_t good_packets_count = 0;
    uint32_t bad_packets_count = 0;
    volatile uint32_t cmd_read_count = 0;
    volatile uint32_t button_bits_sent = 0;

protected:
    void IRAM_ATTR eopHandler();
    char _getChar(uint8_t value);

    int _DATA_PIN;
    int _CLK_PIN;
    int _CS_PIN;

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

    static const uint8_t DSP_CMD1_MODE6_11_7 = 0x01;
    static const uint8_t DSP_CMD1_MODE6_11_7_P05504 = 0x05;
    static const uint8_t DSP_CMD2_DATAREAD = 0x42;
    static const uint8_t DSP_CMD2_DATAWRITE = 0x40;

#ifdef ESP8266
    static const uint32_t PIN_IN = 0x60000318;
    static const uint32_t PIN_OUT_SET = 0x60000304;
    static const uint32_t PIN_OUT_CLEAR = 0x60000308;
    static const uint32_t PIN_DIR_INPUT = 0x60000314;
    static const uint32_t PIN_DIR_OUTPUT = 0x60000310;
#endif

    static const uint8_t CHARCODES[38];
    static const char CHARS[38];
};

class CIO_PRE2021 : public CIO_TYPE1 {
public:
    uint16_t getButtonCode(Buttons button_index) override;
    bool getHasjets() override { return false; }
    bool getHasair() override { return true; }
private:
    static const uint16_t _button_codes[11];
};

extern CIO_TYPE1* g_cio_instance;
}
