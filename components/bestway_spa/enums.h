#pragma once
#include <Arduino.h>

namespace bestway_va {

// Button indices - Updated to match the logic in CIO_TYPE1.cpp
enum Buttons : uint8_t {
    NOBTN = 0,
    LOCK,
    TIMER,
    BUBBLES,
    UNIT,
    HEAT,
    PUMP,
    TEMP_DOWN,  // Renamed from DOWN to match driver logic
    TEMP_UP,    // Renamed from UP to match driver logic
    POWER,
    HYDROJETS,
    BTN_COUNT
};

// State structure - matches VA sStates
struct sStates {
    uint8_t locked = 0;
    uint8_t power = 0;
    uint8_t unit = 0;        // 0 = Celsius, 1 = Fahrenheit
    uint8_t bubbles = 0;
    uint8_t heatgrn = 0;     // Heat standby (green LED)
    uint8_t heatred = 0;     // Heating active (red LED)
    uint8_t heat = 0;        // Combined heat state
    uint8_t pump = 0;        // Filter pump
    uint8_t temperature = 25;
    uint8_t target = 20;
    uint8_t char1 = ' ';
    uint8_t char2 = ' ';
    uint8_t char3 = ' ';
    uint8_t jets = 0;
    uint8_t error = 0;
    uint8_t timerled1 = 0;
    uint8_t timerled2 = 0;
    uint8_t timerbuttonled = 0;
    uint8_t brightness = 8;
    
    // 4-wire specific fields
    bool godmode = false;
    uint8_t no_of_heater_elements_on = 2;
};

// Toggle button indices for 4-wire state machine
enum ToggleButtons : uint8_t {
    BUBBLETOGGLE,
    JETSTOGGLE,
    PUMPTOGGLE,
    HEATTOGGLE
};

}  // namespace bestway_va
