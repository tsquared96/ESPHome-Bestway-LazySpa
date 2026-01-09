#pragma once
#include <Arduino.h>
#include "enums.h"

namespace bestway_va {

class DSP_TYPE1 {
public:
    DSP_TYPE1();
    void setup(int dsp_data_pin, int dsp_clk_pin, int dsp_cs_pin, int dsp_audio_pin);
    void stop();
    void handleStates();
    Buttons getPressedButton();
    virtual Buttons buttonCodeToIndex(uint16_t code) = 0;

    sStates dsp_states;
    String text;
    uint16_t audiofrequency = 0;
    uint32_t good_packets_count = 0;
    uint8_t _raw_payload_from_dsp[2];

protected:
    void uploadPayload(uint8_t brightness);
    uint8_t charTo7SegmCode(char c);
    
    // Marked IRAM_ATTR in the CPP for speed
    void _sendBitsToDSP(uint32_t out_bits, int bit_count);
    uint16_t _receiveBitsFromDSP();
    void clearpayload();

    int _DATA_PIN;
    int _CLK_PIN;
    int _CS_PIN;
    int _AUDIO_PIN;

    unsigned long _dsp_last_refreshtime = 0;
    unsigned long _dsp_getbutton_last_time = 0;
    uint8_t _payload[11] = {0xC0, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00};
    Buttons _old_button = NOBTN;

    static const uint8_t DSP_DIM_BASE = 0x80;
    static const uint8_t DSP_DIM_ON = 0x08;
    static const uint8_t DSP_CMD1_MODE6_11_7 = 0x01;
    static const uint8_t DSP_CMD1_MODE6_11_7_P05504 = 0x05;
    static const uint8_t DSP_CMD2_DATAREAD = 0x42;
    static const uint8_t DSP_CMD2_DATAWRITE = 0x40;

    // Payload byte/bit indices (Keep your existing static constants here...)
    static const uint8_t DGT1_IDX = 1;
    static const uint8_t DGT2_IDX = 3;
    static const uint8_t DGT3_IDX = 5;
    static const uint8_t LCK_IDX = 7;
    static const uint8_t LCK_BIT = 3;
    // ... rest of your indices ...

    // ESP8266 GPIO register addresses for optimized bit-banging
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

class DSP_PRE2021 : public DSP_TYPE1 {
public:
    Buttons buttonCodeToIndex(uint16_t code) override;
private:
    static const uint16_t _button_codes[11];
};

}
