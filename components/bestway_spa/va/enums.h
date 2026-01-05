/**
 * Enums and structures for Bestway Spa control
 *
 * Adapted from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 * https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 *
 * Original work Copyright (c) visualapproach
 * Licensed under GPL v3
 */

#pragma once
#include <Arduino.h>

namespace bestway_va {

// Button indices - matches VisualApproach
enum Buttons : uint8_t {
    NOBTN = 0,
    LOCK,
    TIMER,
    BUBBLES,
    UNIT,
    HEAT,
    PUMP,
    DOWN,
    UP,
    POWER,
    HYDROJETS,
    BTN_COUNT
};

// State indices
enum States : uint8_t {
    LOCKEDSTATE = 0,
    POWERSTATE,
    UNITSTATE,
    BUBBLESSTATE,
    HEATGRNSTATE,
    HEATREDSTATE,
    HEATSTATE,
    PUMPSTATE,
    TEMPERATURE,
    TARGET,
    CHAR1,
    CHAR2,
    CHAR3,
    JETSSTATE,
    ERROR
};

// Model types
enum Models : uint8_t {
    PRE2021,
    MIAMI2021,
    MALDIVES2021,
    M54149E,
    M54173,
    M54154,
    M54144,
    M54138,
    M54123
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
};

// Toggle requests structure - matches VA sToggles
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
};

// Maximum buttons in queue
static const uint8_t MAXBUTTONS = 10;

// Button queue item
struct sButton_queue_item {
    uint16_t btncode;
    int duration_ms;
    uint32_t start_time;
};

}  // namespace bestway_va
