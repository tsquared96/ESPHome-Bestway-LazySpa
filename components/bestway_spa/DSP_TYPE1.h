#ifndef DSP_TYPE1_H
#define DSP_TYPE1_H

#include <Arduino.h>
#include "ports.h"
#include "bestway_va_types.h"

namespace bestway_va {

// The DSP driver handles communication to the physical Display panel
class DSP_TYPE1 {
public:
    void setup(uint8_t data_pin, uint8_t clk_pin, uint8_t cs_pin, int audio_pin);
    void handleStates();
    
    // Interrupt Handlers
    void IRAM_ATTR isr_clkHandler();
    void IRAM_ATTR isr_packetHandler();

    struct {
        char display_text[5]; // e.g., " 38", "FIL", "END"
        bool power_led;
        bool heater_led;
    } dsp_states;

private:
    uint8_t _data_pin, _clk_pin, _cs_pin;
    int _audio_pin;
    uint32_t _data_mask, _clk_mask, _cs_mask;

    volatile uint16_t _captured_words[10];
    volatile uint8_t _bit_idx = 0;
    volatile uint8_t _word_idx = 0;
    
    char _getChar(uint16_t code);
};

} // namespace bestway_va

#endif
