/**
 * Bestway Spa ESPHome Component
 *
 * Modular implementation supporting multiple spa models and protocols:
 * - 6-wire TYPE1 (PRE2021, P05504)
 * - 6-wire TYPE2 (54149E)
 * - 4-wire UART (54123, 54138, 54144, 54154, 54173)
 *
 * Based on VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 * https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 */

#include "bestway_spa.h"
// Use VA source files for 6-wire TYPE1 protocol
#include "CIO_TYPE1.h"
#include "DSP_TYPE1.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <Arduino.h>
#include <cmath>

namespace esphome {
namespace bestway_spa {

static const char *const TAG = "bestway_spa";

// =============================================================================
// TIMING CONSTANTS
// =============================================================================

static const uint32_t STATE_UPDATE_INTERVAL_MS = 500;
static const uint32_t SENSOR_UPDATE_INTERVAL_MS = 2000;
static const uint32_t BUTTON_HOLD_MS = 300;
static const uint32_t STATS_INTERVAL_MS = 5000;
static const uint32_t PACKET_TIMEOUT_MS = 100;

// =============================================================================
// STATIC INSTANCES FOR 6-WIRE PROTOCOLS (using VA source files)
// =============================================================================

// CIO handler instance - using VA PRE2021 implementation
static bestway_va::CIO_PRE2021 va_cio;

// DSP handler instance - using VA PRE2021 implementation
static bestway_va::DSP_PRE2021 va_dsp;

// Active protocol type
static bool is_type1 = true;

// DSP enabled flag (requires separate DSP pins to be configured)
static bool dsp_enabled = false;

// =============================================================================
// SETUP
// =============================================================================

void BestwaySpa::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Bestway Spa...");

  // Set model configuration for 4-wire models
  switch (model_) {
    case MODEL_54123:
      model_config_ = &CONFIG_54123;
      break;
    case MODEL_54138:
      model_config_ = &CONFIG_54138;
      break;
    case MODEL_54144:
      model_config_ = &CONFIG_54144;
      break;
    case MODEL_54154:
      model_config_ = &CONFIG_54154;
      break;
    case MODEL_54173:
      model_config_ = &CONFIG_54173;
      break;
    default:
      model_config_ = &CONFIG_54154;
      break;
  }

  // Initialize 6-wire protocols using modular CIO handlers
  if (protocol_type_ == PROTOCOL_6WIRE_T1 || protocol_type_ == PROTOCOL_6WIRE_T2) {
    // Get pin numbers
    int clk_pin = (cio_clk_pin_ != nullptr) ? cio_clk_pin_->get_pin() : -1;
    int data_pin = (cio_data_pin_ != nullptr) ? cio_data_pin_->get_pin() : -1;
    int cs_pin = (cio_cs_pin_ != nullptr) ? cio_cs_pin_->get_pin() : -1;
    int dsp_pin = (dsp_data_pin_ != nullptr) ? dsp_data_pin_->get_pin() : -1;

    if (dsp_pin < 0) {
      ESP_LOGW(TAG, "DSP_DATA pin not configured - button control will NOT work!");
      ESP_LOGW(TAG, "Add 'dsp_data_pin' to your YAML config for button transmission");
    }

    // Audio pin (optional)
    int audio_gpio = -1;
    if (audio_pin_ != nullptr) {
      audio_gpio = audio_pin_->get_pin();
      pinMode(audio_gpio, OUTPUT);
      digitalWrite(audio_gpio, LOW);
      ESP_LOGD(TAG, "Audio pin configured on GPIO%d", audio_gpio);
    }

    // Initialize CIO handler using VA source (reads from pump controller)
    // VA CIO_TYPE1::setup(data_pin, clk_pin, cs_pin)
    if (protocol_type_ == PROTOCOL_6WIRE_T1) {
      is_type1 = true;
      va_cio.setup(data_pin, clk_pin, cs_pin);

      // Set initial button code to NOBTN
      va_cio.setButtonCode(va_cio.getButtonCode(bestway_va::NOBTN));

      ESP_LOGI(TAG, "6-wire TYPE1 protocol initialized (VA source - PRE2021)");
    } else {
      is_type1 = false;
      // TYPE2 not yet implemented in VA files
      ESP_LOGW(TAG, "TYPE2 protocol not yet implemented in VA files");
    }

    ESP_LOGI(TAG, "  CIO bus (input):  DATA=%d CLK=%d CS=%d", data_pin, clk_pin, cs_pin);

    // Initialize DSP handler for physical display communication (if pins configured)
    int dsp_data_gpio = (dsp_data_pin_ != nullptr) ? dsp_data_pin_->get_pin() : -1;
    int dsp_clk_gpio = (dsp_clk_pin_ != nullptr) ? dsp_clk_pin_->get_pin() : -1;
    int dsp_cs_gpio = (dsp_cs_pin_ != nullptr) ? dsp_cs_pin_->get_pin() : -1;

    if (dsp_data_gpio >= 0 && dsp_clk_gpio >= 0 && dsp_cs_gpio >= 0) {
      // Full DSP bus configured - enable DSP communication with physical display
      // VA DSP_TYPE1::setup(data_pin, clk_pin, cs_pin, audio_pin)
      va_dsp.setup(dsp_data_gpio, dsp_clk_gpio, dsp_cs_gpio, audio_gpio);
      dsp_enabled = true;
      ESP_LOGI(TAG, "DSP bus (physical display): DATA=%d CLK=%d CS=%d",
               dsp_data_gpio, dsp_clk_gpio, dsp_cs_gpio);
    } else {
      dsp_enabled = false;
      ESP_LOGW(TAG, "DSP pins not configured - physical display disabled!");
      ESP_LOGW(TAG, "Add dsp_data_pin, dsp_clk_pin, dsp_cs_pin for display");
    }
  }

  // Initialize climate state
  this->mode = climate::CLIMATE_MODE_OFF;
  this->action = climate::CLIMATE_ACTION_IDLE;
  this->current_temperature = state_.current_temp;
  this->target_temperature = state_.target_temp;

  ESP_LOGCONFIG(TAG, "Bestway Spa initialized");
}

// =============================================================================
// MAIN LOOP
// =============================================================================

void BestwaySpa::loop() {
  const uint32_t now = millis();

  if (paused_) {
    return;
  }

  // Handle protocol based on type
  switch (protocol_type_) {
    case PROTOCOL_4WIRE:
      handle_4wire_protocol_();
      break;
    case PROTOCOL_6WIRE_T1:
    case PROTOCOL_6WIRE_T2:
      handle_6wire_protocol_();
      break;
  }

  // Process button queue and update DSP output for 6-wire protocols
  if (protocol_type_ != PROTOCOL_4WIRE) {
    process_button_queue_();

    // Update the button code in the active CIO handler (VA classes)
    if (is_type1) {
      va_cio.setButtonCode(current_button_code_);
    }
    // TYPE2 not yet implemented
  }

  // Handle toggle requests from HA
  handle_toggles_();

  // Update climate state periodically
  if (now - last_state_update_ > STATE_UPDATE_INTERVAL_MS) {
    update_climate_state_();
    last_state_update_ = now;
  }

  // Update sensors periodically
  if (now - last_sensor_update_ > SENSOR_UPDATE_INTERVAL_MS) {
    update_sensors_();
    last_sensor_update_ = now;
  }

  // Log statistics periodically
  if (now - last_stats_time_ > STATS_INTERVAL_MS) {
    if (protocol_type_ != PROTOCOL_4WIRE && is_type1) {
      uint32_t pkt_delta = va_cio.good_packets_count - last_pkt_count_;
      uint32_t bad_packets = va_cio.bad_packets_count;

      if (dsp_enabled) {
        ESP_LOGI(TAG, "CIO: pkts=%lu(+%lu) bad=%lu | DSP: pkts=%lu | Btn:0x%04X",
                 (unsigned long)va_cio.good_packets_count, (unsigned long)pkt_delta,
                 (unsigned long)bad_packets,
                 (unsigned long)va_dsp.good_packets_count,
                 current_button_code_);
      } else {
        ESP_LOGI(TAG, "CIO: pkts=%lu(+%lu) bad=%lu | Btn:0x%04X",
                 (unsigned long)va_cio.good_packets_count, (unsigned long)pkt_delta,
                 (unsigned long)bad_packets,
                 current_button_code_);
      }

      last_pkt_count_ = va_cio.good_packets_count;
    }
    last_stats_time_ = now;
  }
}

// =============================================================================
// CONFIG DUMP
// =============================================================================

void BestwaySpa::dump_config() {
  ESP_LOGCONFIG(TAG, "Bestway Spa:");

  const char *proto_str;
  switch (protocol_type_) {
    case PROTOCOL_4WIRE:
      proto_str = "4-wire UART";
      break;
    case PROTOCOL_6WIRE_T1:
      proto_str = "6-wire TYPE1 (PRE2021/P05504)";
      break;
    case PROTOCOL_6WIRE_T2:
      proto_str = "6-wire TYPE2 (54149E)";
      break;
    default:
      proto_str = "unknown";
  }
  ESP_LOGCONFIG(TAG, "  Protocol: %s", proto_str);

  const char *model_str;
  switch (model_) {
    case MODEL_PRE2021:
      model_str = "PRE2021";
      break;
    case MODEL_54149E:
      model_str = "54149E";
      break;
    case MODEL_54123:
      model_str = "54123";
      break;
    case MODEL_54138:
      model_str = "54138 (with jets and air)";
      break;
    case MODEL_54144:
      model_str = "54144 (with jets)";
      break;
    case MODEL_54154:
      model_str = "54154";
      break;
    case MODEL_54173:
      model_str = "54173 (with jets and air)";
      break;
    case MODEL_P05504:
      model_str = "P05504";
      break;
    default:
      model_str = "unknown";
  }
  ESP_LOGCONFIG(TAG, "  Model: %s", model_str);
  ESP_LOGCONFIG(TAG, "  Has Jets: %s", has_jets() ? "yes" : "no");
  ESP_LOGCONFIG(TAG, "  Has Air: %s", has_air() ? "yes" : "no");

  if (protocol_type_ != PROTOCOL_4WIRE) {
    ESP_LOGCONFIG(TAG, "  MITM Dual-Bus Architecture:");
    ESP_LOGCONFIG(TAG, "  CIO Bus (input from pump controller):");
    if (cio_clk_pin_ != nullptr)
      ESP_LOGCONFIG(TAG, "    CLK Pin: GPIO%d", cio_clk_pin_->get_pin());
    if (cio_data_pin_ != nullptr)
      ESP_LOGCONFIG(TAG, "    DATA Pin: GPIO%d", cio_data_pin_->get_pin());
    if (cio_cs_pin_ != nullptr)
      ESP_LOGCONFIG(TAG, "    CS Pin: GPIO%d", cio_cs_pin_->get_pin());

    ESP_LOGCONFIG(TAG, "  DSP Bus (output to physical display):");
    if (dsp_enabled) {
      if (dsp_data_pin_ != nullptr)
        ESP_LOGCONFIG(TAG, "    DATA Pin: GPIO%d", dsp_data_pin_->get_pin());
      if (dsp_clk_pin_ != nullptr)
        ESP_LOGCONFIG(TAG, "    CLK Pin: GPIO%d", dsp_clk_pin_->get_pin());
      if (dsp_cs_pin_ != nullptr)
        ESP_LOGCONFIG(TAG, "    CS Pin: GPIO%d", dsp_cs_pin_->get_pin());
      ESP_LOGCONFIG(TAG, "    Status: ENABLED (physical display active)");
    } else {
      ESP_LOGCONFIG(TAG, "    Status: DISABLED (add dsp_data_pin, dsp_clk_pin, dsp_cs_pin)");
    }

    if (is_type1) {
      ESP_LOGCONFIG(TAG, "  CIO Good packets: %lu", (unsigned long)va_cio.good_packets_count);
      ESP_LOGCONFIG(TAG, "  CIO Bad packets: %lu", (unsigned long)va_cio.bad_packets_count);
      if (dsp_enabled) {
        ESP_LOGCONFIG(TAG, "  DSP Good packets: %lu", (unsigned long)va_dsp.good_packets_count);
      }
    }
    // TYPE2 not yet implemented
  }

  LOG_CLIMATE("", "Bestway Spa Climate", this);
}

// =============================================================================
// CLIMATE TRAITS
// =============================================================================

climate::ClimateTraits BestwaySpa::traits() {
  auto traits = climate::ClimateTraits();

  traits.add_supported_mode(climate::CLIMATE_MODE_OFF);
  traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);
  traits.add_supported_mode(climate::CLIMATE_MODE_FAN_ONLY);

  // Always use Celsius range - ESPHome climate works in Celsius internally
  // Home Assistant will display in user's preferred unit
  traits.set_visual_min_temperature(20.0f);
  traits.set_visual_max_temperature(40.0f);
  traits.set_visual_temperature_step(1.0f);

  // Support both Celsius and Fahrenheit display
  traits.set_supports_current_temperature(true);

  return traits;
}

// =============================================================================
// CLIMATE CONTROL
// =============================================================================

void BestwaySpa::control(const climate::ClimateCall &call) {
  if (call.get_mode().has_value()) {
    climate::ClimateMode mode = *call.get_mode();

    switch (mode) {
      case climate::CLIMATE_MODE_OFF:
        set_heater(false);
        set_filter(false);
        break;

      case climate::CLIMATE_MODE_HEAT:
        set_heater(true);
        set_filter(true);
        break;

      case climate::CLIMATE_MODE_FAN_ONLY:
        set_heater(false);
        set_filter(true);
        break;

      default:
        break;
    }
  }

  if (call.get_target_temperature().has_value()) {
    float new_target = *call.get_target_temperature();
    set_target_temp(new_target);
  }
}

// =============================================================================
// 4-WIRE UART PROTOCOL HANDLER
// =============================================================================

void BestwaySpa::handle_4wire_protocol_() {
  while (available()) {
    uint8_t byte;
    read_byte(&byte);
    rx_buffer_.push_back(byte);
    last_packet_time_ = millis();
  }

  if (rx_buffer_.size() >= 7) {
    if (rx_buffer_[0] == 0xFF && rx_buffer_[6] == 0xFF) {
      uint8_t calc_sum = rx_buffer_[1] + rx_buffer_[2] + rx_buffer_[3] + rx_buffer_[4];
      if (calc_sum == rx_buffer_[5]) {
        parse_4wire_packet_(rx_buffer_);
        new_packet_available_ = true;
      } else {
        ESP_LOGW(TAG, "4-wire checksum mismatch: calc=%02X, recv=%02X", calc_sum, rx_buffer_[5]);
      }
      rx_buffer_.erase(rx_buffer_.begin(), rx_buffer_.begin() + 7);
    } else {
      rx_buffer_.erase(rx_buffer_.begin());
    }
  }

  if (!rx_buffer_.empty() && (millis() - last_packet_time_) > PACKET_TIMEOUT_MS) {
    ESP_LOGV(TAG, "4-wire packet timeout, clearing %d bytes", rx_buffer_.size());
    rx_buffer_.clear();
  }

  if (new_packet_available_) {
    send_4wire_response_();
    new_packet_available_ = false;
  }
}

void BestwaySpa::parse_4wire_packet_(const std::vector<uint8_t> &packet) {
  if (packet.size() < 7) return;

  uint8_t command = packet[1];
  uint8_t temp_raw = packet[2];
  uint8_t error = packet[3];

  state_.current_temp = (float)temp_raw;
  state_.error_code = error;

  state_.filter_pump = (command & model_config_->pump_bitmask) != 0;
  state_.bubbles = (command & model_config_->bubbles_bitmask) != 0;
  if (model_config_->has_jets) {
    state_.jets = (command & model_config_->jets_bitmask) != 0;
  }

  bool heat1 = (command & model_config_->heat_bitmask1) != 0;
  bool heat2 = (command & model_config_->heat_bitmask2) != 0;
  state_.heater_red = heat1 || heat2;
  state_.heater_green = state_.heater_enabled && !state_.heater_red;
  state_.heater_enabled = state_.heater_red || state_.heater_green;

  ESP_LOGV(TAG, "4-wire: cmd=%02X temp=%d err=%d pump=%d bubbles=%d heat=%d",
           command, temp_raw, error, state_.filter_pump, state_.bubbles, state_.heater_red);
}

void BestwaySpa::send_4wire_response_() {
  uint8_t packet[7];
  packet[0] = 0xFF;

  uint8_t command = 0;

  if (state_.heater_enabled) {
    uint32_t now = millis();
    if (heater_stage_ == 0) {
      heater_stage_ = 1;
      stage_start_time_ = now;
      command |= model_config_->heat_bitmask1;
    } else if (heater_stage_ == 1) {
      command |= model_config_->heat_bitmask1;
      if ((now - stage_start_time_) > 10000) {
        heater_stage_ = 2;
      }
    } else {
      command |= model_config_->heat_bitmask1;
      command |= model_config_->heat_bitmask2;
    }
  } else {
    heater_stage_ = 0;
  }

  if (state_.filter_pump) {
    command |= model_config_->pump_bitmask;
  }
  if (state_.bubbles) {
    command |= model_config_->bubbles_bitmask;
  }
  if (model_config_->has_jets && state_.jets) {
    command |= model_config_->jets_bitmask;
  }

  packet[1] = command;
  packet[2] = (uint8_t)state_.target_temp;
  packet[3] = 0x00;
  packet[4] = 0x00;
  packet[5] = packet[1] + packet[2] + packet[3] + packet[4];
  packet[6] = 0xFF;

  write_array(packet, 7);
  flush();
}

// =============================================================================
// 6-WIRE PROTOCOL HANDLER - Uses VA source files directly
// Based on VA firmware BWC::loop():
//   cio->updateStates();                // Parse CIO packets (ISR fills buffer)
//   dsp->dsp_states = cio->cio_states;  // Copy states to DSP
//   dsp->handleStates();                // Send to physical display
//   button = dsp->getPressedButton();   // Read button from physical display
//   cio->setButtonCode();               // Set button for transmission
// =============================================================================

void BestwaySpa::handle_6wire_protocol_() {
  if (!is_type1) {
    // TYPE2 not yet implemented
    return;
  }

  // =========================================================================
  // STEP 1: cio->updateStates() - Process CIO packets received via ISR
  // The ISR fills _payload buffer, updateStates() parses it into cio_states
  // =========================================================================
  va_cio.updateStates();

  // Check if we got new packet(s) by comparing packet count
  if (va_cio.good_packets_count > good_packets_) {
    good_packets_ = va_cio.good_packets_count;

    // Copy VA cio_states to our SpaState
    const bestway_va::sStates& cio = va_cio.cio_states;

    // Parse into pending state for debouncing
    SpaState new_state;
    new_state.power = cio.power;
    new_state.heater_red = cio.heatred;
    new_state.heater_green = cio.heatgrn;
    new_state.heater_enabled = cio.heat;
    new_state.filter_pump = cio.pump;
    new_state.bubbles = cio.bubbles;
    new_state.jets = cio.jets;
    new_state.locked = cio.locked;
    new_state.unit_celsius = (cio.unit == 0);  // 0 = C, 1 = F
    new_state.error_code = cio.error;

    // Temperature from VA (already parsed from 7-segment)
    float temp = (float)cio.temperature;
    if (!new_state.unit_celsius && temp > 60.0f) {
      // Convert F to C for internal storage
      temp = (temp - 32.0f) * 5.0f / 9.0f;
    }
    new_state.current_temp = temp;

    // Display characters
    new_state.display_chars[0] = (char)cio.char1;
    new_state.display_chars[1] = (char)cio.char2;
    new_state.display_chars[2] = (char)cio.char3;
    new_state.display_chars[3] = '\0';

    // Debounce: Check if new state matches pending state
    bool states_match =
        (new_state.power == pending_state_.power) &&
        (new_state.heater_red == pending_state_.heater_red) &&
        (new_state.heater_green == pending_state_.heater_green) &&
        (new_state.filter_pump == pending_state_.filter_pump) &&
        (new_state.bubbles == pending_state_.bubbles) &&
        (new_state.jets == pending_state_.jets) &&
        (new_state.locked == pending_state_.locked) &&
        (std::fabs(new_state.current_temp - pending_state_.current_temp) < 1.0f);

    if (states_match) {
      state_match_count_++;
    } else {
      pending_state_ = new_state;
      state_match_count_ = 1;
    }

    // Update actual state when we have consistent readings
    if (state_match_count_ >= STATE_DEBOUNCE_COUNT) {
      state_.power = new_state.power;
      state_.heater_red = new_state.heater_red;
      state_.heater_green = new_state.heater_green;
      state_.heater_enabled = new_state.heater_enabled;
      state_.filter_pump = new_state.filter_pump;
      state_.bubbles = new_state.bubbles;
      state_.jets = new_state.jets;
      state_.locked = new_state.locked;
      state_.unit_celsius = new_state.unit_celsius;
      state_.current_temp = new_state.current_temp;
      state_.error_code = new_state.error_code;
      state_.display_chars[0] = new_state.display_chars[0];
      state_.display_chars[1] = new_state.display_chars[1];
      state_.display_chars[2] = new_state.display_chars[2];
      state_.display_chars[3] = '\0';
      state_.brightness = va_cio.brightness;

      ESP_LOGD(TAG, "CIO: '%c%c%c' T:%.0f Pwr:%d Heat:%d Pump:%d",
               state_.display_chars[0], state_.display_chars[1], state_.display_chars[2],
               state_.current_temp,
               state_.power ? 1 : 0,
               state_.heater_enabled ? 1 : 0,
               state_.filter_pump ? 1 : 0);
    }
  }

  // =========================================================================
  // STEP 2 & 3: Copy CIO states to DSP and send to physical display
  // =========================================================================
  if (dsp_enabled) {
    // Copy CIO states to DSP (this is what VA does)
    va_dsp.dsp_states = va_cio.cio_states;
    va_dsp.dsp_states.brightness = state_.brightness;  // Allow HA override

    // Send to physical display at ~20Hz (rate-limited inside handleStates)
    va_dsp.handleStates();
  }

  // =========================================================================
  // STEP 4: Read button presses from physical display
  // =========================================================================
  if (dsp_enabled) {
    bestway_va::Buttons button = va_dsp.getPressedButton();

    if (button != bestway_va::NOBTN) {
      ESP_LOGI(TAG, "Physical display button: %d", button);

      // Convert VA button enum to ESPHome button enum (they're identical)
      Buttons esphome_btn = static_cast<Buttons>(button);

      if (is_button_enabled(esphome_btn)) {
        queue_button_(esphome_btn);
      } else {
        ESP_LOGI(TAG, "Button %d disabled, ignoring", button);
      }
    }
  }

  // =========================================================================
  // STEP 5: Button transmission to CIO is handled by:
  // - process_button_queue_() sets current_button_code_
  // - loop() calls va_cio.setButtonCode(current_button_code_)
  // - CIO ISR transmits button code when pump controller requests it (0x42)
  // =========================================================================
}

// parse_6wire_cio_packet_ removed - parsing now done by VA's updateStates()

// =============================================================================
// DATA PIN HELPERS
// =============================================================================

bool BestwaySpa::read_data_pin_() {
  if (cio_data_pin_ != nullptr) {
    return cio_data_pin_->digital_read();
  }
  return false;
}

void BestwaySpa::write_data_pin_(bool value) {
  if (dsp_data_pin_ != nullptr) {
    dsp_data_pin_->digital_write(value);
  }
}

// =============================================================================
// STATE MANAGEMENT
// =============================================================================

void BestwaySpa::handle_toggles_() {
  if (toggles_.power_pressed) {
    queue_button_(POWER);
    toggles_.power_pressed = false;
  }

  if (toggles_.lock_pressed) {
    queue_button_(LOCK);
    toggles_.lock_pressed = false;
  }

  if (toggles_.heat_pressed) {
    queue_button_(HEAT);
    toggles_.heat_pressed = false;
  }

  if (toggles_.pump_pressed) {
    queue_button_(PUMP);
    toggles_.pump_pressed = false;
  }

  if (toggles_.bubbles_pressed) {
    queue_button_(BUBBLES);
    toggles_.bubbles_pressed = false;
  }

  if (toggles_.jets_pressed && has_jets()) {
    queue_button_(HYDROJETS);
    toggles_.jets_pressed = false;
  }

  if (toggles_.unit_pressed) {
    queue_button_(UNIT);
    toggles_.unit_pressed = false;
  }

  if (toggles_.timer_pressed) {
    queue_button_(TIMER);
    toggles_.timer_pressed = false;
  }

  if (toggles_.set_target_temp && toggles_.target_temp_delta != 0) {
    Buttons btn = toggles_.target_temp_delta > 0 ? UP : DOWN;
    int steps = abs(toggles_.target_temp_delta);
    for (int i = 0; i < steps; i++) {
      queue_button_(btn);
    }
    toggles_.target_temp_delta = 0;
    toggles_.set_target_temp = false;
  }
}

void BestwaySpa::update_climate_state_() {
  this->current_temperature = state_.current_temp;
  this->target_temperature = state_.target_temp;

  if (!state_.power) {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->action = climate::CLIMATE_ACTION_OFF;
  } else if (state_.heater_enabled) {
    this->mode = climate::CLIMATE_MODE_HEAT;

    if (state_.heater_red) {
      this->action = climate::CLIMATE_ACTION_HEATING;
    } else if (state_.heater_green) {
      this->action = climate::CLIMATE_ACTION_IDLE;
    } else {
      this->action = climate::CLIMATE_ACTION_IDLE;
    }
  } else if (state_.filter_pump) {
    this->mode = climate::CLIMATE_MODE_FAN_ONLY;
    this->action = climate::CLIMATE_ACTION_FAN;
  } else {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->action = climate::CLIMATE_ACTION_IDLE;
  }

  this->publish_state();
}

void BestwaySpa::update_sensors_() {
  if (current_temp_sensor_ != nullptr) {
    current_temp_sensor_->publish_state(state_.current_temp);
  }

  if (target_temp_sensor_ != nullptr) {
    target_temp_sensor_->publish_state(state_.target_temp);
  }

  if (heating_sensor_ != nullptr) {
    heating_sensor_->publish_state(state_.heater_red);
  }

  if (filter_sensor_ != nullptr) {
    filter_sensor_->publish_state(state_.filter_pump);
  }

  if (bubbles_sensor_ != nullptr) {
    bubbles_sensor_->publish_state(state_.bubbles);
  }

  if (jets_sensor_ != nullptr) {
    jets_sensor_->publish_state(state_.jets);
  }

  if (locked_sensor_ != nullptr) {
    locked_sensor_->publish_state(state_.locked);
  }

  if (power_sensor_ != nullptr) {
    power_sensor_->publish_state(state_.power);
  }

  if (error_sensor_ != nullptr) {
    error_sensor_->publish_state(state_.error_code != 0);
  }

  if (error_text_sensor_ != nullptr) {
    if (state_.error_code != 0) {
      char error_str[8];
      snprintf(error_str, sizeof(error_str), "E%02d", state_.error_code);
      error_text_sensor_->publish_state(error_str);
    } else {
      error_text_sensor_->publish_state("OK");
    }
  }

  if (display_text_sensor_ != nullptr) {
    display_text_sensor_->publish_state(std::string(state_.display_chars));
  }

  if (button_status_sensor_ != nullptr) {
    // Convert button code to name
    const char* btn_name = "UNKNOWN";
    if (current_button_code_ == get_button_code_(NOBTN)) {
      btn_name = "NOBTN";
    } else if (current_button_code_ == get_button_code_(LOCK)) {
      btn_name = "LOCK";
    } else if (current_button_code_ == get_button_code_(TIMER)) {
      btn_name = "TIMER";
    } else if (current_button_code_ == get_button_code_(BUBBLES)) {
      btn_name = "BUBBLES";
    } else if (current_button_code_ == get_button_code_(UNIT)) {
      btn_name = "UNIT";
    } else if (current_button_code_ == get_button_code_(HEAT)) {
      btn_name = "HEAT";
    } else if (current_button_code_ == get_button_code_(PUMP)) {
      btn_name = "PUMP";
    } else if (current_button_code_ == get_button_code_(DOWN)) {
      btn_name = "DOWN";
    } else if (current_button_code_ == get_button_code_(UP)) {
      btn_name = "UP";
    } else if (current_button_code_ == get_button_code_(POWER)) {
      btn_name = "POWER";
    } else if (current_button_code_ == get_button_code_(HYDROJETS)) {
      btn_name = "JETS";
    }
    button_status_sensor_->publish_state(btn_name);
  }
}

// =============================================================================
// BUTTON QUEUE FOR 6-WIRE
// =============================================================================

void BestwaySpa::queue_button_(Buttons button, int duration_ms) {
  if (!button_enabled_[button]) {
    ESP_LOGI(TAG, "Button %d is disabled, ignoring", button);
    return;
  }

  ButtonQueueItem item;
  item.button_code = get_button_code_(button);
  item.duration_ms = duration_ms;
  item.start_time = 0;
  item.target_state = 0xFF;
  item.target_value = 0;

  // Check for invalid button code (0x0000 = not implemented for this model)
  if (item.button_code == 0x0000) {
    ESP_LOGW(TAG, "Button %d has no valid code (0x0000) for this model", button);
    return;
  }

  button_queue_.push_back(item);
  ESP_LOGI(TAG, "Queued button %d (code 0x%04X) for %dms", button, item.button_code, duration_ms);
}

void BestwaySpa::process_button_queue_() {
  if (button_queue_.empty()) {
    current_button_code_ = get_button_code_(NOBTN);
    return;
  }

  uint32_t now = millis();
  auto &item = button_queue_.front();

  if (item.start_time == 0) {
    item.start_time = now;
    current_button_code_ = item.button_code;
    ESP_LOGI(TAG, "Pressing button 0x%04X", item.button_code);
  }

  if ((now - item.start_time) >= (uint32_t)item.duration_ms) {
    ESP_LOGD(TAG, "Released button 0x%04X", item.button_code);
    button_queue_.erase(button_queue_.begin());
  }
}

uint16_t BestwaySpa::get_button_code_(Buttons button) {
  if (button >= BTN_COUNT) {
    // Return NOBTN for invalid button
    return (model_ == MODEL_54149E) ? 0x0000 : 0x1B1B;
  }

  // Use button codes from bestway_spa.h
  switch (model_) {
    case MODEL_PRE2021:
      return BTN_CODES_PRE2021[button];
    case MODEL_P05504:
      return BTN_CODES_P05504[button];
    case MODEL_54149E:
      return BTN_CODES_54149E[button];
    default:
      return BTN_CODES_PRE2021[button];
  }
}

// Character decoding removed - now done by VA's _getChar() method

// =============================================================================
// CONTROL METHODS
// =============================================================================

void BestwaySpa::set_power(bool state) {
  if (protocol_type_ == PROTOCOL_4WIRE) {
    if (state_.power != state) {
      state_.power = state;
    }
  } else {
    // 6-wire: Always queue button press - it's a toggle
    toggles_.power_pressed = true;
    ESP_LOGI(TAG, "Requested power toggle (want %s)", state ? "ON" : "OFF");
  }
}

void BestwaySpa::set_heater(bool state) {
  if (protocol_type_ == PROTOCOL_4WIRE) {
    if (state_.heater_enabled != state) {
      state_.heater_enabled = state;
      if (state && !state_.filter_pump) {
        state_.filter_pump = true;
      }
    }
  } else {
    // 6-wire: Always queue button press - it's a toggle
    toggles_.heat_pressed = true;
    ESP_LOGI(TAG, "Requested heater toggle (want %s)", state ? "ON" : "OFF");
  }
}

void BestwaySpa::set_filter(bool state) {
  if (protocol_type_ == PROTOCOL_4WIRE) {
    if (state_.filter_pump != state) {
      state_.filter_pump = state;
      if (!state && state_.heater_enabled) {
        state_.heater_enabled = false;
      }
    }
  } else {
    // 6-wire: Always queue button press - it's a toggle
    toggles_.pump_pressed = true;
    ESP_LOGI(TAG, "Requested filter toggle (want %s)", state ? "ON" : "OFF");
  }
}

void BestwaySpa::set_bubbles(bool state) {
  if (protocol_type_ == PROTOCOL_4WIRE) {
    if (state_.bubbles != state) {
      state_.bubbles = state;
    }
  } else {
    // 6-wire: Always queue button press - it's a toggle
    toggles_.bubbles_pressed = true;
    ESP_LOGI(TAG, "Requested bubbles toggle (want %s)", state ? "ON" : "OFF");
  }
}

void BestwaySpa::set_jets(bool state) {
  if (!has_jets()) {
    ESP_LOGW(TAG, "This model does not have jets");
    return;
  }

  if (protocol_type_ == PROTOCOL_4WIRE) {
    if (state_.jets != state) {
      state_.jets = state;
    }
  } else {
    // 6-wire: Always queue button press - it's a toggle
    toggles_.jets_pressed = true;
    ESP_LOGI(TAG, "Requested jets toggle (want %s)", state ? "ON" : "OFF");
  }
}

void BestwaySpa::set_lock(bool state) {
  if (protocol_type_ == PROTOCOL_4WIRE) {
    if (state_.locked != state) {
      state_.locked = state;
    }
  } else {
    // 6-wire: Always queue button press - it's a toggle
    toggles_.lock_pressed = true;
    ESP_LOGI(TAG, "Requested lock toggle (want %s)", state ? "ON" : "OFF");
  }
}

void BestwaySpa::set_unit(bool celsius) {
  if (protocol_type_ == PROTOCOL_4WIRE) {
    if (state_.unit_celsius != celsius) {
      state_.unit_celsius = celsius;
      if (celsius) {
        state_.current_temp = fahrenheit_to_celsius_(state_.current_temp);
        state_.target_temp = fahrenheit_to_celsius_(state_.target_temp);
      } else {
        state_.current_temp = celsius_to_fahrenheit_(state_.current_temp);
        state_.target_temp = celsius_to_fahrenheit_(state_.target_temp);
      }
    }
  } else {
    // 6-wire: Always queue button press - it's a toggle
    toggles_.unit_pressed = true;
    ESP_LOGI(TAG, "Requested unit toggle (want %s)", celsius ? "C" : "F");
  }
}

void BestwaySpa::set_target_temp(float temp) {
  float delta = temp - state_.target_temp;
  int8_t steps = (int8_t)roundf(delta);

  if (steps != 0) {
    adjust_target_temp(steps);
  }
}

void BestwaySpa::adjust_target_temp(int8_t delta) {
  if (delta == 0) return;

  if (protocol_type_ == PROTOCOL_4WIRE) {
    state_.target_temp += delta;
    if (state_.unit_celsius) {
      if (state_.target_temp < 20.0f) state_.target_temp = 20.0f;
      if (state_.target_temp > 40.0f) state_.target_temp = 40.0f;
    } else {
      if (state_.target_temp < 68.0f) state_.target_temp = 68.0f;
      if (state_.target_temp > 104.0f) state_.target_temp = 104.0f;
    }
  } else {
    toggles_.set_target_temp = true;
    toggles_.target_temp_delta = delta;
    ESP_LOGD(TAG, "Adjusting target temperature by %d steps", delta);
  }
}

void BestwaySpa::set_timer(uint8_t hours) {
  ESP_LOGD(TAG, "Setting timer to %d hours", hours);
  if (protocol_type_ != PROTOCOL_4WIRE) {
    toggles_.timer_pressed = true;
  }
  state_.timer_hours = hours;
}

void BestwaySpa::set_brightness(uint8_t level) {
  if (level > 8) level = 8;
  state_.brightness = level;
  ESP_LOGD(TAG, "Set brightness to %d", level);
}

void BestwaySpa::set_button_enabled(Buttons btn, bool enabled) {
  if (btn >= BTN_COUNT) {
    ESP_LOGW(TAG, "Invalid button index %d", btn);
    return;
  }
  button_enabled_[btn] = enabled;
  ESP_LOGI(TAG, "Button %d %s", btn, enabled ? "enabled" : "disabled");
}

bool BestwaySpa::is_button_enabled(Buttons btn) const {
  if (btn >= BTN_COUNT) {
    return false;
  }
  return button_enabled_[btn];
}

// =============================================================================
// STATE GETTERS
// =============================================================================

bool BestwaySpa::has_jets() const {
  switch (model_) {
    case MODEL_54138:
    case MODEL_54144:
    case MODEL_54173:
    case MODEL_54149E:
      return true;
    default:
      return false;
  }
}

bool BestwaySpa::has_air() const {
  switch (model_) {
    case MODEL_PRE2021:
    case MODEL_54138:
    case MODEL_54173:
    case MODEL_54149E:
    case MODEL_P05504:
      return true;
    case MODEL_54123:
    case MODEL_54144:
    case MODEL_54154:
      return false;
    default:
      return true;
  }
}

// =============================================================================
// UTILITIES
// =============================================================================

uint8_t BestwaySpa::calculate_checksum_(const uint8_t *data, size_t len) {
  uint8_t sum = 0;
  for (size_t i = 0; i < len; i++) {
    sum += data[i];
  }
  return sum;
}

}  // namespace bestway_spa
}  // namespace esphome
