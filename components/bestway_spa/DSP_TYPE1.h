#pragma once
#include <Arduino.h>
#include "enums.h"

// ESP8266 Register access for high-speed bit-banging
#ifndef PIN_IN
#define PIN_IN          0x60000318
#define PIN_OUT_SET     0x60000304
#define PIN_OUT_CLEAR   0x60000308
#define PIN_DIR_OUTPUT  0x60000300
#define PIN_DIR_INPUT   0x60000300
#endif

namespace bestway_va {

class DSP_TYPE1 {
public:
    DSP_TYPE1();
    void setup(int dsp_data_pin, int dsp_clk_pin, int dsp_cs_pin, int audio_pin = -1);
    void handleStates();
    Buttons getPressedButton();

    // Interrupt handlers must be IRAM_ATTR and public for the glue code
    void IRAM_ATTR isr_packetHandler();
    void IRAM_ATTR isr_clkHandler();

    // This is shared with the CIO driver to sync the display state
    sStates dsp_states;

protected:
    int _DATA_PIN;
    int _CLK_PIN;
    int _CS_PIN;
    int _AUDIO_PIN;

    // Volatile variables modified inside the ISRs
    volatile uint8_t _byte_count;
    volatile uint8_t _bit_count;
    volatile uint8_t _received_byte;
    volatile uint8_t _payload[11];
    volatile bool _packet_transm_active;
    volatile bool _new_packet_available;

    Buttons _last_btn{NOBTN};
};

} // namespace bestway_va
