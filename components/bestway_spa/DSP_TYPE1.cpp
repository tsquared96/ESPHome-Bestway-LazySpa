#include "DSP_TYPE1.h"

namespace bestway_va {

// ... [Keep CHARCODES, CHARS, and _button_codes arrays exactly as they are] ...

// OPTIMIZED: Send bits using direct register manipulation
void IRAM_ATTR DSP_TYPE1::_sendBitsToDSP(uint32_t outBits, int bitsToSend) {
    // Set Data Pin to Output
    WRITE_PERI_REG(PIN_DIR_OUTPUT, (READ_PERI_REG(PIN_DIR_OUTPUT) | (1 << _DATA_PIN)));
    
    for (int i = 0; i < bitsToSend; i++) {
        // Clock LOW
        WRITE_PERI_REG(PIN_OUT_CLEAR, 1 << _CLK_PIN);
        
        // Set Data Bit
        if (outBits & (1 << i)) {
            WRITE_PERI_REG(PIN_OUT_SET, 1 << _DATA_PIN);
        } else {
            WRITE_PERI_REG(PIN_OUT_CLEAR, 1 << _DATA_PIN);
        }
        
        delayMicroseconds(15); // Slightly tighter timing for 160MHz
        
        // Clock HIGH
        WRITE_PERI_REG(PIN_OUT_SET, 1 << _CLK_PIN);
        delayMicroseconds(15);
    }
}

// OPTIMIZED: Receive bits using direct register manipulation
uint16_t IRAM_ATTR DSP_TYPE1::_receiveBitsFromDSP() {
    uint16_t result = 0;
    // Set Data Pin to Input
    WRITE_PERI_REG(PIN_DIR_INPUT, (1 << _DATA_PIN));

    for (int i = 0; i < 16; i++) {
        WRITE_PERI_REG(PIN_OUT_CLEAR, 1 << _CLK_PIN);
        delayMicroseconds(15);
        
        WRITE_PERI_REG(PIN_OUT_SET, 1 << _CLK_PIN);
        delayMicroseconds(15);
        
        // Bit mapping: 8-15 then 0-7 per VA protocol
        int j = (i + 8) % 16; 
        if (READ_PERI_REG(PIN_IN) & (1 << _DATA_PIN)) {
            result |= (1 << j);
        }
    }
    return result;
}

// OPTIMIZED: Upload payload with interrupt protection
void DSP_TYPE1::uploadPayload(uint8_t brightness) {
    if (millis() - _dsp_last_refreshtime < 60) return; // Increased refresh rate slightly (16Hz)
    _dsp_last_refreshtime = millis();

    uint8_t enableLED = (brightness > 0) ? DSP_DIM_ON : 0;
    uint8_t brightVal = (brightness > 0) ? (brightness - 1) : 0;

    // CRITICAL: We wrap the transmission in a "noInterrupts" block
    // This prevents WiFi from breaking the display timing
    noInterrupts();

    // Packet 1: Mode
    WRITE_PERI_REG(PIN_OUT_CLEAR, 1 << _CS_PIN);
    _sendBitsToDSP(DSP_CMD1_MODE6_11_7_P05504, 8);
    WRITE_PERI_REG(PIN_OUT_SET, 1 << _CS_PIN);
    delayMicroseconds(40);

    // Packet 2: Data write command
    WRITE_PERI_REG(PIN_OUT_CLEAR, 1 << _CS_PIN);
    _sendBitsToDSP(DSP_CMD2_DATAWRITE, 8);
    WRITE_PERI_REG(PIN_OUT_SET, 1 << _CS_PIN);
    delayMicroseconds(40);

    // Packet 3: The 11-byte status payload
    WRITE_PERI_REG(PIN_OUT_CLEAR, 1 << _CS_PIN);
    for (int i = 0; i < 11; i++) {
        _sendBitsToDSP(_payload[i], 8);
    }
    WRITE_PERI_REG(PIN_OUT_SET, 1 << _CS_PIN);
    delayMicroseconds(40);

    // Packet 4: Brightness
    WRITE_PERI_REG(PIN_OUT_CLEAR, 1 << _CS_PIN);
    _sendBitsToDSP(DSP_DIM_BASE | enableLED | brightVal, 8);
    WRITE_PERI_REG(PIN_OUT_SET, 1 << _CS_PIN);

    interrupts(); // Re-enable WiFi/ESPHome tasks
}

// ... [Keep getPressedButton, clearpayload, and handleStates as they were] ...

} // namespace bestway_va
