#ifndef CIO_TYPE1_H
#define CIO_TYPE1_H

#include <Arduino.h>
#include "ports.h"
#include "bestway_va_types.h"

namespace bestway_va {

// The CIO driver handles communication from the Pump/Heater unit
class CIO_PRE2021 {
public:
    void setup(uint8_t data_pin, uint8_t clk_pin, uint8_t cs_pin);
    void updateStates();
    
    // Interrupt Handlers
    void IRAM_ATTR isr_clkHandler();
    void IRAM_ATTR isr_packetHandler();

    // Button Injection
    uint16_t getButtonCode(Buttons btn);
    void setButtonCode(uint16_t code);

    struct {
        uint8_t temperature;
        uint8_t target;
        bool heater;
        bool filter;
        bool bubbles;
    } cio_states;

private:
    uint8_t _data_pin, _clk_pin, _cs_pin;
    uint32_t _data_mask, _clk_mask, _cs_mask;
    
    volatile uint16_t _incoming_packet[6];
    volatile uint16_t _outgoing_button_code = 0x1B1B; // Default NOBTN
    volatile uint8_t _bit_count = 0;
    volatile uint8_t _word_count = 0;
};

} // namespace bestway_va

#endif
