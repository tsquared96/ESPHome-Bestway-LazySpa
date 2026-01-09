#include "CIO_TYPE1.h"

namespace bestway_va {

CIO_TYPE1* g_cio_instance = nullptr;

static void IRAM_ATTR isr_cs_wrapper() { if (g_cio_instance) g_cio_instance->isr_packetHandler(); }
static void IRAM_ATTR isr_clk_wrapper() { if (g_cio_instance) g_cio_instance->isr_clkHandler(); }

// ... [Keep CHARCODES and CHARS arrays from your previous version] ...

void IRAM_ATTR CIO_TYPE1::isr_clkHandler() {
    if (!_packet_transm_active) return;

    // Direct register read for instantaneous state capture
    uint32_t gpio_status = READ_PERI_REG(PIN_IN);
    bool clockstate = gpio_status & (1 << _CLK_PIN);

    // 1. FALLING EDGE: Send button bits to the pump
    if (!clockstate && _data_is_output) {
        if (_button_code & (1 << _send_bit)) {
            WRITE_PERI_REG(PIN_OUT_SET, 1 << _DATA_PIN);
        } else {
            WRITE_PERI_REG(PIN_OUT_CLEAR, 1 << _DATA_PIN);
        }
        _send_bit++;
        if (_send_bit > 15) _send_bit = 0;
        return; 
    }

    // 2. RISING EDGE: Read data from the pump
    if (clockstate && !_data_is_output) {
        _received_byte = (_received_byte >> 1) | (((gpio_status & (1 << _DATA_PIN)) > 0) << 7);
        _bit_count++;
        
        if (_bit_count == 8) {
            _bit_count = 0;

            if (_CIO_cmd_matches == 2 && _byte_count < 11) {
                _payload[_byte_count++] = _received_byte;
            } 
            // THE CRITICAL FIX: Faster transition to output mode
            else if (_received_byte == DSP_CMD2_DATAREAD) {
                _send_bit = 8; 
                _data_is_output = true;
                
                // Set Pin to Output
                WRITE_PERI_REG(PIN_DIR_OUTPUT, 1 << _DATA_PIN);
                
                // Push first bit IMMEDIATELY before the next falling edge
                if (_button_code & (1 << _send_bit)) {
                    WRITE_PERI_REG(PIN_OUT_SET, 1 << _DATA_PIN);
                } else {
                    WRITE_PERI_REG(PIN_OUT_CLEAR, 1 << _DATA_PIN);
                }
                _send_bit++;
                cmd_read_count++;
            }
        }
    }
}

// ... [Keep updateStates and eopHandler logic] ...

}
