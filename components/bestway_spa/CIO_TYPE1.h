#pragma once
#include <Arduino.h>
#include "enums.h"

namespace bestway_va {

class CIO_TYPE1 {
public:
    CIO_TYPE1();
    void setup(int cio_data_pin, int cio_clk_pin, int cio_cs_pin);
    void updateStates();
    void setButtonCode(uint16_t code) { _button_code = code; }
    virtual uint16_t getButtonCode(Buttons button_index) = 0;
    
    void IRAM_ATTR isr_packetHandler();
    void IRAM_ATTR isr_clkHandler();

    sStates cio_states;
    uint8_t brightness = 7; // Must be public
    uint32_t good_packets_count = 0;

protected:
    void IRAM_ATTR eopHandler();
    int _DATA_PIN;
    int _CLK_PIN;
    int _CS_PIN;
    volatile uint8_t _brightness;
    volatile uint8_t _payload[11];
    volatile bool _new_packet_available;
    volatile uint16_t _button_code;
    
    // ... [Rest of internal variables] ...
};

class CIO_PRE2021 : public CIO_TYPE1 {
public:
    uint16_t getButtonCode(Buttons button_index) override;
};

extern CIO_TYPE1* g_cio_instance;
}
