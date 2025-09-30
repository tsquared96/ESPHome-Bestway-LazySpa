// spa_protocol.h
// Custom protocol implementation for Bestway Lay-Z-SPA communication
// Based on reverse-engineered protocol from WiFi-remote-for-Bestway-Lay-Z-SPA

#ifndef SPA_PROTOCOL_H
#define SPA_PROTOCOL_H

#include "esphome.h"

// Protocol timing constants (microseconds)
#define CLK_PERIOD_US 100
#define CS_HOLD_US 50
#define BIT_DELAY_US 10

// Button codes for spa control
enum SpaButton {
    BTN_NONE = 0x00,
    BTN_TEMP_UP = 0x01,
    BTN_TEMP_DOWN = 0x02,
    BTN_UNIT = 0x04,
    BTN_LOCK = 0x08,
    BTN_HEATER = 0x10,
    BTN_FILTER = 0x20,
    BTN_BUBBLES = 0x40,
    BTN_JETS = 0x80,
    BTN_POWER = 0xFF
};

// Status flags
enum SpaStatus {
    STATUS_HEATER = 0x01,
    STATUS_FILTER = 0x02,
    STATUS_BUBBLES = 0x04,
    STATUS_JETS = 0x08,
    STATUS_LOCKED = 0x80
};

// Error codes
enum SpaError {
    ERROR_NONE = 0x00,
    ERROR_FLOW = 0x01,
    ERROR_TEMP_SENSOR = 0x02,
    ERROR_OVERHEAT = 0x03,
    ERROR_FREEZE = 0x04,
    ERROR_DRY_HEAT = 0x05,
    ERROR_COMM = 0x06
};

// Packet structure for DSP communication
struct DspPacket {
    uint8_t start;          // Start byte (0xAA)
    uint8_t temp_high;      // Temperature high byte
    uint8_t temp_low;       // Temperature low byte  
    uint8_t button;         // Button press code
    uint8_t status;         // Status flags
    uint8_t error;          // Error code
    uint8_t display[3];     // 7-segment display data
    uint8_t reserved;       // Reserved byte
    uint8_t checksum;       // XOR checksum
};

// Packet structure for CIO communication
struct CioPacket {
    uint8_t start;          // Start byte (0x55)
    uint8_t target_high;    // Target temp high byte
    uint8_t target_low;     // Target temp low byte
    uint8_t command;        // Command flags
    uint8_t button;         // Button to simulate
    uint8_t reserved[5];    // Reserved bytes
    uint8_t checksum;       // XOR checksum
};

// Helper class for protocol handling
class SpaProtocol {
public:
    // Calculate XOR checksum for packet
    static uint8_t calculate_checksum(uint8_t* data, size_t len) {
        uint8_t checksum = 0;
        for (size_t i = 0; i < len - 1; i++) {
            checksum ^= data[i];
        }
        return checksum;
    }
    
    // Validate packet checksum
    static bool validate_packet(uint8_t* data, size_t len) {
        if (len < 2) return false;
        uint8_t calc_checksum = calculate_checksum(data, len);
        return calc_checksum == data[len - 1];
    }
    
    // Convert temperature to protocol format (x10)
    static uint16_t temp_to_protocol(float temp_c) {
        return (uint16_t)(temp_c * 10);
    }
    
    // Convert protocol format to temperature
    static float protocol_to_temp(uint16_t proto_temp) {
        return proto_temp / 10.0f;
    }
    
    // Decode 7-segment display byte
    static char decode_7segment(uint8_t segment) {
        // 7-segment to ASCII mapping
        const uint8_t segments[] = {
            0x3F, // 0
            0x06, // 1
            0x5B, // 2
            0x4F, // 3
            0x66, // 4
            0x6D, // 5
            0x7D, // 6
            0x07, // 7
            0x7F, // 8
            0x6F, // 9
            0x77, // A
            0x7C, // b
            0x39, // C
            0x5E, // d
            0x79, // E
            0x71  // F
        };
        
        for (int i = 0; i < 16; i++) {
            if (segments[i] == segment) {
                return i < 10 ? '0' + i : 'A' + (i - 10);
            }
        }
        return ' ';
    }
    
    // Encode ASCII to 7-segment
    static uint8_t encode_7segment(char c) {
        const uint8_t segments[] = {
            0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
            0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71
        };
        
        if (c >= '0' && c <= '9') {
            return segments[c - '0'];
        } else if (c >= 'A' && c <= 'F') {
            return segments[10 + (c - 'A')];
        } else if (c >= 'a' && c <= 'f') {
            return segments[10 + (c - 'a')];
        }
        return 0x00; // Blank
    }
    
    // Create display message for 3-digit display
    static void create_display_message(const char* text, uint8_t* display) {
        for (int i = 0; i < 3; i++) {
            display[i] = text[i] ? encode_7segment(text[i]) : 0x00;
        }
    }
    
    // Parse DSP packet
    static bool parse_dsp_packet(uint8_t* data, DspPacket* packet) {
        if (data[0] != 0xAA) return false;
        
        memcpy(packet, data, sizeof(DspPacket));
        return validate_packet(data, sizeof(DspPacket));
    }
    
    // Build CIO packet
    static void build_cio_packet(CioPacket* packet, float target_temp, uint8_t command, uint8_t button) {
        packet->start = 0x55;
        
        uint16_t temp = temp_to_protocol(target_temp);
        packet->target_high = (temp >> 8) & 0xFF;
        packet->target_low = temp & 0xFF;
        
        packet->command = command;
        packet->button = button;
        
        memset(packet->reserved, 0, sizeof(packet->reserved));
        
        packet->checksum = calculate_checksum((uint8_t*)packet, sizeof(CioPacket));
    }
};

// Bit-banging SPI communication class
class BitBangSPI {
private:
    GPIOPin* data_pin;
    GPIOPin* clk_pin;
    GPIOPin* cs_pin;
    bool is_output;
    
public:
    BitBangSPI(GPIOPin* data, GPIOPin* clk, GPIOPin* cs, bool output = true) 
        : data_pin(data), clk_pin(clk), cs_pin(cs), is_output(output) {
        
        if (is_output) {
            data_pin->setup();
            data_pin->pin_mode(gpio::FLAG_OUTPUT);
        } else {
            data_pin->setup();
            data_pin->pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
        }
        
        clk_pin->setup();
        clk_pin->pin_mode(is_output ? gpio::FLAG_OUTPUT : gpio::FLAG_INPUT);
        
        cs_pin->setup();
        cs_pin->pin_mode(is_output ? gpio::FLAG_OUTPUT : gpio::FLAG_INPUT);
        
        if (is_output) {
            cs_pin->digital_write(true);
            clk_pin->digital_write(false);
        }
    }
    
    // Write a byte (MSB first)
    void write_byte(uint8_t data) {
        if (!is_output) return;
        
        for (int bit = 7; bit >= 0; bit--) {
            clk_pin->digital_write(false);
            data_pin->digital_write((data >> bit) & 0x01);
            delayMicroseconds(BIT_DELAY_US);
            clk_pin->digital_write(true);
            delayMicroseconds(BIT_DELAY_US);
        }
        clk_pin->digital_write(false);
    }
    
    // Read a byte (MSB first)
    uint8_t read_byte() {
        if (is_output) return 0;
        
        uint8_t data = 0;
        for (int bit = 7; bit >= 0; bit--) {
            // Wait for clock high
            while (!clk_pin->digital_read()) {
                yield();
                delayMicroseconds(1);
            }
            
            // Read bit
            if (data_pin->digital_read()) {
                data |= (1 << bit);
            }
            
            // Wait for clock low
            while (clk_pin->digital_read()) {
                yield();
                delayMicroseconds(1);
            }
        }
        return data;
    }
    
    // Write a packet
    void write_packet(uint8_t* data, size_t len) {
        if (!is_output) return;
        
        cs_pin->digital_write(false);
        delayMicroseconds(CS_HOLD_US);
        
        for (size_t i = 0; i < len; i++) {
            write_byte(data[i]);
        }
        
        delayMicroseconds(CS_HOLD_US);
        cs_pin->digital_write(true);
    }
    
    // Read a packet (non-blocking)
    bool read_packet(uint8_t* buffer, size_t len, uint32_t timeout_ms = 100) {
        if (is_output) return false;
        
        uint32_t start = millis();
        
        // Wait for CS low
        while (cs_pin->digital_read()) {
            if (millis() - start > timeout_ms) return false;
            yield();
            delayMicroseconds(10);
        }
        
        // Read data
        for (size_t i = 0; i < len; i++) {
            buffer[i] = read_byte();
            if (millis() - start > timeout_ms) return false;
        }
        
        // Wait for CS high
        while (!cs_pin->digital_read()) {
            if (millis() - start > timeout_ms) return false;
            yield();
            delayMicroseconds(10);
        }
        
        return true;
    }
    
    // Check if CS is active (low)
    bool is_selected() {
        return !cs_pin->digital_read();
    }
};

// Model-specific configuration
class ModelConfig {
public:
    enum Model {
        MODEL_6WIRE_2021,
        MODEL_6WIRE_PRE2021,
        MODEL_4WIRE_2021,
        MODEL_4WIRE_PRE2021
    };
    
    static bool has_cio(Model model) {
        return model == MODEL_6WIRE_2021 || model == MODEL_6WIRE_PRE2021;
    }
    
    static bool has_jets(Model model) {
        // Some models have jets feature
        return model == MODEL_6WIRE_2021;
    }
    
    static uint8_t get_button_code(Model model, SpaButton button) {
        // Button codes may vary by model
        // This is a simplified mapping
        switch (model) {
            case MODEL_4WIRE_2021:
            case MODEL_4WIRE_PRE2021:
                // 4-wire models may have different codes
                switch (button) {
                    case BTN_HEATER: return 0x08;
                    case BTN_FILTER: return 0x04;
                    case BTN_BUBBLES: return 0x02;
                    default: return button;
                }
            default:
                return button;
        }
    }
    
    static const char* get_model_name(Model model) {
        switch (model) {
            case MODEL_6WIRE_2021: return "6-Wire 2021";
            case MODEL_6WIRE_PRE2021: return "6-Wire Pre-2021";
            case MODEL_4WIRE_2021: return "4-Wire 2021";
            case MODEL_4WIRE_PRE2021: return "4-Wire Pre-2021";
            default: return "Unknown";
        }
    }
};

#endif // SPA_PROTOCOL_H