#pragma once
/**
 * Model-Specific Button Codes - Ported from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 *
 * This file contains button code mappings for all supported spa models.
 * Each model has a unique set of button codes that correspond to physical buttons.
 *
 * Based on: https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 * Original author: visualapproach
 *
 * NOTE: Button and State enums are defined in bestway_spa.h to avoid conflicts.
 * This file only contains the button CODE ARRAYS for each model.
 * Must be included AFTER bestway_spa.h
 */

#include <stdint.h>

namespace esphome {
namespace bestway_spa {

// =============================================================================
// BUTTON CODES BY MODEL - From VisualApproach
// =============================================================================

// Index order: NOBTN, LOCK, TIMER, BUBBLES, UNIT, HEAT, PUMP, DOWN, UP, POWER, JETS

/**
 * PRE2021 (6-wire TYPE1)
 * Model name in VA: "PRE2021" / "NO6"
 * Protocol: 6-wire TYPE1
 */
static const uint16_t BTNCODES_PRE2021[BTN_COUNT] = {
  0x1B1B,  // NOBTN
  0x0200,  // LOCK
  0x0100,  // TIMER
  0x0300,  // BUBBLES
  0x1012,  // UNIT
  0x1212,  // HEAT
  0x1112,  // PUMP
  0x1312,  // DOWN
  0x0809,  // UP
  0x0000,  // POWER (not used on PRE2021 - always on)
  0x0000   // JETS (not available on PRE2021)
};

/**
 * P05504 (6-wire TYPE1 variant)
 * Model name in VA: "P05504"
 * Protocol: 6-wire TYPE1
 */
static const uint16_t BTNCODES_P05504[BTN_COUNT] = {
  0x1B1B,  // NOBTN
  0x0210,  // LOCK
  0x0110,  // TIMER
  0x0310,  // BUBBLES
  0x1022,  // UNIT
  0x1222,  // HEAT
  0x1122,  // PUMP
  0x1322,  // DOWN
  0x081A,  // UP
  0x0000,  // POWER (not used)
  0x0000   // JETS (not available)
};

/**
 * MODEL 54149E (6-wire TYPE2)
 * Model name in VA: "NO54149E" / "54149E"
 * Protocol: 6-wire TYPE2
 */
static const uint16_t BTNCODES_54149E[BTN_COUNT] = {
  0x0000,  // NOBTN (all zeros for TYPE2)
  0x0080,  // LOCK
  0x0040,  // TIMER
  0x0020,  // BUBBLES
  0x0010,  // UNIT
  0x0008,  // HEAT
  0x0004,  // PUMP
  0x0002,  // DOWN
  0x0001,  // UP
  0x0100,  // POWER
  0x0200   // JETS
};

/**
 * MODEL 54123 (4-wire)
 * Model name in VA: "NO54123"
 * Protocol: 4-wire UART
 * Features: No jets, no air
 */
static const uint16_t BTNCODES_54123[BTN_COUNT] = {
  0x0000,  // NOBTN
  0x0000,  // LOCK (4-wire handled differently)
  0x0000,  // TIMER
  0x0000,  // BUBBLES
  0x0000,  // UNIT
  0x0000,  // HEAT
  0x0000,  // PUMP
  0x0000,  // DOWN
  0x0000,  // UP
  0x0000,  // POWER
  0x0000   // JETS
};

/**
 * MODEL 54138 (4-wire)
 * Model name in VA: "NO54138"
 * Protocol: 4-wire UART
 * Features: Has jets and air
 */
static const uint16_t BTNCODES_54138[BTN_COUNT] = {
  0x0000,  // NOBTN
  0x0000,  // LOCK
  0x0000,  // TIMER
  0x0000,  // BUBBLES
  0x0000,  // UNIT
  0x0000,  // HEAT
  0x0000,  // PUMP
  0x0000,  // DOWN
  0x0000,  // UP
  0x0000,  // POWER
  0x0000   // JETS
};

/**
 * MODEL 54144 (4-wire)
 * Model name in VA: "NO54144"
 * Protocol: 4-wire UART
 * Features: Has jets, no air
 */
static const uint16_t BTNCODES_54144[BTN_COUNT] = {
  0x0000,  // NOBTN
  0x0000,  // LOCK
  0x0000,  // TIMER
  0x0000,  // BUBBLES
  0x0000,  // UNIT
  0x0000,  // HEAT
  0x0000,  // PUMP
  0x0000,  // DOWN
  0x0000,  // UP
  0x0000,  // POWER
  0x0000   // JETS
};

/**
 * MODEL 54154 (4-wire)
 * Model name in VA: "NO54154"
 * Protocol: 4-wire UART
 * Features: No jets, no air
 */
static const uint16_t BTNCODES_54154[BTN_COUNT] = {
  0x0000,  // NOBTN
  0x0000,  // LOCK
  0x0000,  // TIMER
  0x0000,  // BUBBLES
  0x0000,  // UNIT
  0x0000,  // HEAT
  0x0000,  // PUMP
  0x0000,  // DOWN
  0x0000,  // UP
  0x0000,  // POWER
  0x0000   // JETS
};

/**
 * MODEL 54173 (4-wire)
 * Model name in VA: "NO54173"
 * Protocol: 4-wire UART
 * Features: Has jets and air
 */
static const uint16_t BTNCODES_54173[BTN_COUNT] = {
  0x0000,  // NOBTN
  0x0000,  // LOCK
  0x0000,  // TIMER
  0x0000,  // BUBBLES
  0x0000,  // UNIT
  0x0000,  // HEAT
  0x0000,  // PUMP
  0x0000,  // DOWN
  0x0000,  // UP
  0x0000,  // POWER
  0x0000   // JETS
};

// =============================================================================
// 4-WIRE MODEL BITMASK CONFIGURATIONS - From VisualApproach
// =============================================================================

/**
 * 4-wire models use UART protocol where control is done via
 * command bytes with bitmasks rather than button codes.
 */

struct Model4WireConfig {
  // Heater control bitmasks (dual-stage heating)
  uint8_t heat_bitmask1;   // Heater stage 1
  uint8_t heat_bitmask2;   // Heater stage 2
  // Feature bitmasks
  uint8_t pump_bitmask;    // Filter pump
  uint8_t bubbles_bitmask; // Air bubbles
  uint8_t jets_bitmask;    // HydroJets
  // Feature availability
  bool has_jets;
  bool has_air;
};

// Model configurations (from VisualApproach)
static const Model4WireConfig CONFIG_4W_54123 = {
  .heat_bitmask1 = 0x02,
  .heat_bitmask2 = 0x08,
  .pump_bitmask = 0x04,
  .bubbles_bitmask = 0x10,
  .jets_bitmask = 0x00,
  .has_jets = false,
  .has_air = false
};

static const Model4WireConfig CONFIG_4W_54138 = {
  .heat_bitmask1 = 0x30,
  .heat_bitmask2 = 0x40,
  .pump_bitmask = 0x04,
  .bubbles_bitmask = 0x08,
  .jets_bitmask = 0x80,
  .has_jets = true,
  .has_air = true
};

static const Model4WireConfig CONFIG_4W_54144 = {
  .heat_bitmask1 = 0x30,
  .heat_bitmask2 = 0x40,
  .pump_bitmask = 0x04,
  .bubbles_bitmask = 0x08,
  .jets_bitmask = 0x80,
  .has_jets = true,
  .has_air = false
};

static const Model4WireConfig CONFIG_4W_54154 = {
  .heat_bitmask1 = 0x02,
  .heat_bitmask2 = 0x08,
  .pump_bitmask = 0x04,
  .bubbles_bitmask = 0x10,
  .jets_bitmask = 0x00,
  .has_jets = false,
  .has_air = false
};

static const Model4WireConfig CONFIG_4W_54173 = {
  .heat_bitmask1 = 0x30,
  .heat_bitmask2 = 0x40,
  .pump_bitmask = 0x04,
  .bubbles_bitmask = 0x08,
  .jets_bitmask = 0x80,
  .has_jets = true,
  .has_air = true
};

// =============================================================================
// 6-WIRE MODEL FEATURES
// =============================================================================

// PRE2021 features
static const bool PRE2021_HAS_JETS = false;
static const bool PRE2021_HAS_AIR = true;

// P05504 features
static const bool P05504_HAS_JETS = false;
static const bool P05504_HAS_AIR = true;

// 54149E features
static const bool MODEL54149E_HAS_JETS = true;
static const bool MODEL54149E_HAS_AIR = true;

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

/**
 * Get button code array for a given model
 */
inline const uint16_t* getButtonCodesForModel(uint8_t model) {
  switch (model) {
    case 0:  // MODEL_PRE2021
      return BTNCODES_PRE2021;
    case 1:  // MODEL_54149E
      return BTNCODES_54149E;
    case 7:  // MODEL_P05504
      return BTNCODES_P05504;
    // 4-wire models don't use button codes
    default:
      return BTNCODES_PRE2021;
  }
}

/**
 * Get 4-wire config for a given model
 */
inline const Model4WireConfig* get4WireConfigForModel(uint8_t model) {
  switch (model) {
    case 2:  // MODEL_54123
      return &CONFIG_4W_54123;
    case 3:  // MODEL_54138
      return &CONFIG_4W_54138;
    case 4:  // MODEL_54144
      return &CONFIG_4W_54144;
    case 5:  // MODEL_54154
      return &CONFIG_4W_54154;
    case 6:  // MODEL_54173
      return &CONFIG_4W_54173;
    default:
      return &CONFIG_4W_54154;
  }
}

}  // namespace bestway_spa
}  // namespace esphome
