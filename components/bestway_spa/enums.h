#pragma once
#include <Arduino.h>

namespace bestway_va {

// Button indices
enum Buttons : uint8_t {
    NOBTN = 0,
    LOCK,
    TIMER,
    BUBBLES,
    UNIT,
    HEAT,
    PUMP,
    TEMP_DOWN, 
    TEMP_UP,   
    POWER,
    HYDROJETS,
    BTN_COUNT
};

// State structure
struct sStates {
    uint8_t locked = 0;
    uint8_t power = 0;
    uint8_t unit = 0;
    uint8_t bubbles = 0;
    uint8_t heatgrn = 0;
    uint8_t heatred = 0;
    uint8_t heat = 0;
    uint8_t pump = 0;
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
    bool godmode = false;
    uint8_t no_of_heater_elements_on = 2;
};

// ADDED: Toggle requests structure (Required by 4-wire/Type 2 files)
struct sToggles {
    Buttons pressed_button = NOBTN;
    uint8_t target = 20;
    bool locked_pressed = false;
    bool power_change = false;
    bool unit_change = false;
    bool bubbles_change = false;
    bool heat_change = false;
    bool pump_change = false;
    bool jets_change = false;
    bool timer_pressed = false;
    bool up_pressed = false;
    bool down_pressed = false;
    bool godmode = false;
    uint8_t no_of_heater_elements_on = 2;
};

enum ToggleButtons : uint8_t {
    BUBBLETOGGLE,
    JETSTOGGLE,
    PUMPTOGGLE,
    HEATTOGGLE
};

// ADDED: Global character array (Required by DSP_TYPE2)
static const uint8_t CHARS[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ' ', '-', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
    'h', 'H', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'x', 'y', 'z'
};

}  // namespace bestway_va
