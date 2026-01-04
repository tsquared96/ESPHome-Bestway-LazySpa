#include "bestway_spa.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace bestway_spa {

static const char *const TAG = "bestway_spa";

// Timing constants
static const uint32_t PACKET_TIMEOUT_MS = 100;
static const uint32_t STATE_UPDATE_INTERVAL_MS = 100;
static const uint32_t SENSOR_UPDATE_INTERVAL_MS = 2000;

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

  // Initialize 6-wire TYPE1 protocol
  if (protocol_type_ == PROTOCOL_6WIRE_T1) {
    // Get pin numbers
    int cio_data = cio_data_pin_ ? cio_data_pin_->get_pin() : -1;
    int cio_clk = cio_clk_pin_ ? cio_clk_pin_->get_pin() : -1;
    int cio_cs = cio_cs_pin_ ? cio_cs_pin_->get_pin() : -1;

    // DSP pins - use same as CIO if not specified (man-in-the-middle setup)
    int dsp_data = dsp_data_pin_ ? dsp_data_pin_->get_pin() : cio_data;
    int dsp_clk = dsp_clk_pin_ ? dsp_clk_pin_->get_pin() : cio_clk;
    int dsp_cs = dsp_cs_pin_ ? dsp_cs_pin_->get_pin() : cio_cs;
    int audio = audio_pin_ ? audio_pin_->get_pin() : -1;

    if (cio_data < 0 || cio_clk < 0 || cio_cs < 0) {
      ESP_LOGE(TAG, "6-wire TYPE1 requires data, clk, and cs pins!");
      return;
    }

    // Set button codes based on model
    if (model_ == MODEL_P05504) {
      cio_type1_.set_button_codes(BTN_CODES_P05504, BTN_COUNT);
    } else {
      cio_type1_.set_button_codes(BTN_CODES_PRE2021, BTN_COUNT);
    }

    // Set model features
    cio_type1_.set_has_jets(has_jets());
    cio_type1_.set_has_air(has_air());

    // Check if DSP pins are separate from CIO pins
    bool has_separate_dsp_pins = (dsp_data_pin_ != nullptr) && (dsp_clk_pin_ != nullptr) && (dsp_cs_pin_ != nullptr);

    // Initialize CIO handler (interrupt-driven, receives FROM CIO board)
    cio_type1_.setup(cio_data, cio_clk, cio_cs);

    // Only initialize DSP handler if we have separate pins (man-in-the-middle setup)
    if (has_separate_dsp_pins) {
      dsp_type1_.setup(dsp_data, dsp_clk, dsp_cs, audio);
      dsp_initialized_ = true;
      ESP_LOGI(TAG, "DSP handler initialized with separate pins");
    } else {
      dsp_initialized_ = false;
      ESP_LOGW(TAG, "No separate DSP pins configured - DSP output disabled");
      ESP_LOGW(TAG, "For man-in-middle setup, configure dsp_clk_pin, dsp_data_pin, dsp_cs_pin");
    }

    ESP_LOGI(TAG, "6-wire TYPE1 initialized - CIO: data=%d clk=%d cs=%d, DSP: data=%d clk=%d cs=%d audio=%d",
             cio_data, cio_clk, cio_cs, dsp_data, dsp_clk, dsp_cs, audio);
  }

  // Initialize climate state - default to power ON and heating enabled
  state_.power = true;
  state_.heater_enabled = true;
  state_.filter_pump = true;

  this->mode = climate::CLIMATE_MODE_HEAT;
  this->action = climate::CLIMATE_ACTION_IDLE;
  this->current_temperature = state_.current_temp;
  this->target_temperature = state_.target_temp;

  const char *proto_str;
  switch (protocol_type_) {
    case PROTOCOL_4WIRE:
      proto_str = "4-wire UART";
      break;
    case PROTOCOL_6WIRE_T1:
      proto_str = "6-wire TYPE1";
      break;
    case PROTOCOL_6WIRE_T2:
      proto_str = "6-wire TYPE2";
      break;
    default:
      proto_str = "unknown";
  }

  ESP_LOGCONFIG(TAG, "Bestway Spa initialized (Protocol: %s)", proto_str);
}

// =============================================================================
// MAIN LOOP
// =============================================================================

void BestwaySpa::loop() {
  const uint32_t now = millis();

  // Handle protocol based on type
  switch (protocol_type_) {
    case PROTOCOL_4WIRE:
      handle_4wire_protocol_();
      break;
    case PROTOCOL_6WIRE_T1:
      handle_6wire_type1_protocol_();
      break;
    case PROTOCOL_6WIRE_T2:
      handle_6wire_type2_protocol_();
      break;
  }

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
      proto_str = "6-wire TYPE1 (SPI-like, interrupt-driven)";
      break;
    case PROTOCOL_6WIRE_T2:
      proto_str = "6-wire TYPE2 (SPI-like)";
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

  if (protocol_type_ == PROTOCOL_6WIRE_T1) {
    if (cio_clk_pin_ != nullptr)
      ESP_LOGCONFIG(TAG, "  CIO CLK Pin: GPIO%d", cio_clk_pin_->get_pin());
    if (cio_data_pin_ != nullptr)
      ESP_LOGCONFIG(TAG, "  CIO DATA Pin: GPIO%d", cio_data_pin_->get_pin());
    if (cio_cs_pin_ != nullptr)
      ESP_LOGCONFIG(TAG, "  CIO CS Pin: GPIO%d", cio_cs_pin_->get_pin());
    ESP_LOGCONFIG(TAG, "  CIO Good packets: %u", cio_type1_.get_good_packets());
    ESP_LOGCONFIG(TAG, "  CIO Bad packets: %u", cio_type1_.get_bad_packets());
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

  // Temperature range depends on unit
  if (state_.unit_celsius) {
    traits.set_visual_min_temperature(20.0f);
    traits.set_visual_max_temperature(40.0f);
  } else {
    traits.set_visual_min_temperature(68.0f);
    traits.set_visual_max_temperature(104.0f);
  }

  traits.set_visual_temperature_step(1.0f);

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
// 6-WIRE TYPE1 PROTOCOL HANDLER (Interrupt-based)
// =============================================================================

void BestwaySpa::handle_6wire_type1_protocol_() {
  // Periodic diagnostic logging (every 5 seconds)
  static uint32_t last_diag_time = 0;
  static uint32_t last_good_packets = 0;
  static uint32_t last_cs_int = 0;
  static uint32_t last_clk_int = 0;
  if (millis() - last_diag_time > 5000) {
    uint32_t good = cio_type1_.get_good_packets();
    uint32_t bad = cio_type1_.get_bad_packets();
    uint32_t cs_int = cio_type1_.get_cs_interrupts();
    uint32_t clk_int = cio_type1_.get_clk_interrupts();

    // Read raw pin states for debugging
    int cs_pin = cio_cs_pin_ ? cio_cs_pin_->get_pin() : -1;
    int clk_pin = cio_clk_pin_ ? cio_clk_pin_->get_pin() : -1;
    int data_pin = cio_data_pin_ ? cio_data_pin_->get_pin() : -1;
    int cs_state = (cs_pin >= 0) ? digitalRead(cs_pin) : -1;
    int clk_state = (clk_pin >= 0) ? digitalRead(clk_pin) : -1;
    int data_state = (data_pin >= 0) ? digitalRead(data_pin) : -1;

    ESP_LOGI(TAG, "CIO: pkts=%u(+%u) bad=%u | ISR: cs=%u(+%u) clk=%u(+%u) | Pins: cs=%d clk=%d data=%d",
             good, good - last_good_packets, bad,
             cs_int, cs_int - last_cs_int, clk_int, clk_int - last_clk_int,
             cs_state, clk_state, data_state);
    last_good_packets = good;
    last_cs_int = cs_int;
    last_clk_int = clk_int;
    last_diag_time = millis();
  }

  // Check for new packet BEFORE update_states() clears the flag
  bool has_new_data = cio_type1_.is_new_packet_available();

  // Update CIO states from interrupt-received data
  cio_type1_.update_states();

  // If we had new data, sync to DSP (and our internal state)
  if (has_new_data) {
    sync_cio_to_dsp_();
  }

  // DSP operations only if DSP is initialized
  if (dsp_initialized_) {
    // Get button presses from DSP and handle them
    Buttons button = dsp_type1_.get_pressed_button();
    if (button != NOBTN) {
      handle_button_press_(button);
    }

    // Update DSP display with current states
    dsp_type1_.handle_states();
  }

  // Handle pending button from ESPHome controls
  if (pending_button_ != NOBTN) {
    // Set the button code in CIO handler to simulate button press
    cio_type1_.set_button_code(cio_type1_.get_button_code(pending_button_));
    pending_button_ = NOBTN;
  }

  // Handle temperature adjustment steps
  if (temp_adjust_steps_ != 0) {
    Buttons btn = temp_adjust_steps_ > 0 ? UP : DOWN;
    cio_type1_.set_button_code(cio_type1_.get_button_code(btn));
    if (temp_adjust_steps_ > 0) {
      temp_adjust_steps_--;
    } else {
      temp_adjust_steps_++;
    }
  }
}

void BestwaySpa::sync_cio_to_dsp_() {
  // Get CIO states
  CIOStates &cio = cio_type1_.get_states();
  DSPStates &dsp = dsp_type1_.get_states();

  // Debug: log raw payload and decoded characters
  uint8_t *raw = cio_type1_.get_raw_payload();
  ESP_LOGD(TAG, "CIO Raw: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
           raw[0], raw[1], raw[2], raw[3], raw[4], raw[5], raw[6], raw[7], raw[8], raw[9], raw[10]);
  ESP_LOGD(TAG, "CIO Chars: '%c' '%c' '%c' Temp:%d Power:%d Heat:%d Pump:%d",
           cio.char1, cio.char2, cio.char3, cio.temperature, cio.power, cio.heat, cio.pump);

  // Copy state from CIO to DSP
  dsp.locked = cio.locked;
  dsp.power = cio.power;
  dsp.unit = cio.unit;
  dsp.bubbles = cio.bubbles;
  dsp.heatgrn = cio.heatgrn;
  dsp.heatred = cio.heatred;
  dsp.pump = cio.pump;
  dsp.jets = cio.jets;
  dsp.timerled1 = cio.timerled1;
  dsp.timerled2 = cio.timerled2;
  dsp.timerbuttonled = cio.timerbuttonled;
  dsp.char1 = cio.char1;
  dsp.char2 = cio.char2;
  dsp.char3 = cio.char3;

  // Update our internal state
  state_.locked = cio.locked;
  state_.power = cio.power;
  state_.unit_celsius = cio.unit;
  state_.bubbles = cio.bubbles;
  state_.heater_green = cio.heatgrn;
  state_.heater_red = cio.heatred;
  state_.heater_enabled = cio.heat;
  state_.filter_pump = cio.pump;
  state_.jets = cio.jets;
  state_.current_temp = (float)cio.temperature;
  state_.target_temp = (float)cio.target;
  state_.error_code = cio.error;

  // Update display chars
  state_.display_chars[0] = cio.char1;
  state_.display_chars[1] = cio.char2;
  state_.display_chars[2] = cio.char3;
  state_.display_chars[3] = '\0';

  // Handle power on pending request
  if (power_on_pending_ && !state_.power) {
    pending_button_ = POWER;
    power_on_pending_ = false;
  }
}

void BestwaySpa::handle_button_press_(Buttons button) {
  ESP_LOGD(TAG, "Button pressed: %d", button);

  // Forward the button press to CIO
  cio_type1_.set_button_code(cio_type1_.get_button_code(button));
}

// =============================================================================
// 6-WIRE TYPE2 PROTOCOL HANDLER (placeholder)
// =============================================================================

void BestwaySpa::handle_6wire_type2_protocol_() {
  // TYPE2 implementation placeholder
  ESP_LOGW(TAG, "6-wire TYPE2 protocol not yet implemented with interrupts");
}

// =============================================================================
// 4-WIRE UART PROTOCOL HANDLER
// =============================================================================

void BestwaySpa::handle_4wire_protocol_() {
  // Read available bytes from UART
  while (available()) {
    uint8_t byte;
    read_byte(&byte);
    rx_buffer_.push_back(byte);
    last_packet_time_ = millis();
  }

  // Process complete packets (7-byte fixed format)
  if (rx_buffer_.size() >= 7) {
    // Check for valid packet start/end markers
    if (rx_buffer_[0] == 0xFF && rx_buffer_[6] == 0xFF) {
      // Validate checksum
      uint8_t calc_sum = rx_buffer_[1] + rx_buffer_[2] + rx_buffer_[3] + rx_buffer_[4];
      if (calc_sum == rx_buffer_[5]) {
        parse_4wire_packet_(rx_buffer_);
      } else {
        ESP_LOGW(TAG, "4-wire checksum mismatch: calc=%02X, recv=%02X", calc_sum, rx_buffer_[5]);
      }
      rx_buffer_.erase(rx_buffer_.begin(), rx_buffer_.begin() + 7);
    } else {
      // Invalid packet, skip first byte
      rx_buffer_.erase(rx_buffer_.begin());
    }
  }

  // Clear buffer on timeout
  if (!rx_buffer_.empty() && (millis() - last_packet_time_) > PACKET_TIMEOUT_MS) {
    ESP_LOGV(TAG, "4-wire packet timeout, clearing %d bytes", rx_buffer_.size());
    rx_buffer_.clear();
  }

  // Send response
  send_4wire_response_();
}

void BestwaySpa::parse_4wire_packet_(const std::vector<uint8_t> &packet) {
  if (packet.size() < 7) return;

  // Extract command byte and temperature
  uint8_t command = packet[1];
  uint8_t temp_raw = packet[2];
  uint8_t error = packet[3];

  // Parse temperature (raw value is actual temperature)
  state_.current_temp = (float)temp_raw;

  // Parse error code
  state_.error_code = error;

  // Parse state flags from command byte
  state_.filter_pump = (command & model_config_->pump_bitmask) != 0;
  state_.bubbles = (command & model_config_->bubbles_bitmask) != 0;
  if (model_config_->has_jets) {
    state_.jets = (command & model_config_->jets_bitmask) != 0;
  }

  // Parse heater state (dual stage)
  bool heat1 = (command & model_config_->heat_bitmask1) != 0;
  bool heat2 = (command & model_config_->heat_bitmask2) != 0;
  state_.heater_red = heat1 || heat2;
  state_.heater_green = state_.heater_enabled && !state_.heater_red;

  ESP_LOGV(TAG, "4-wire: cmd=%02X temp=%d err=%d pump=%d bubbles=%d heat=%d",
           command, temp_raw, error, state_.filter_pump, state_.bubbles, state_.heater_red);
}

void BestwaySpa::send_4wire_response_() {
  // Build response packet
  uint8_t packet[7];
  packet[0] = 0xFF;  // Start marker

  // Command byte with requested states
  uint8_t command = 0;

  // Heater control with staged startup
  if (state_.heater_enabled) {
    uint32_t now = millis();
    if (heater_stage_ == 0) {
      // Start stage 1
      heater_stage_ = 1;
      stage_start_time_ = now;
      command |= model_config_->heat_bitmask1;
    } else if (heater_stage_ == 1) {
      // Stage 1 active, check if time to advance
      command |= model_config_->heat_bitmask1;
      if ((now - stage_start_time_) > 10000) {  // 10 second delay
        heater_stage_ = 2;
      }
    } else {
      // Both stages active
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
  packet[2] = (uint8_t)state_.target_temp;  // Target temperature
  packet[3] = 0x00;  // Reserved
  packet[4] = 0x00;  // Reserved
  packet[5] = packet[1] + packet[2] + packet[3] + packet[4];  // Checksum
  packet[6] = 0xFF;  // End marker

  write_array(packet, 7);
  flush();
}

// =============================================================================
// STATE MANAGEMENT
// =============================================================================

void BestwaySpa::update_climate_state_() {
  // Check if state has actually changed
  bool state_changed = false;

  if (this->current_temperature != state_.current_temp) {
    this->current_temperature = state_.current_temp;
    state_changed = true;
  }

  if (this->target_temperature != state_.target_temp) {
    this->target_temperature = state_.target_temp;
    state_changed = true;
  }

  // Determine mode based on state
  climate::ClimateMode new_mode;
  climate::ClimateAction new_action;

  if (!state_.power) {
    new_mode = climate::CLIMATE_MODE_OFF;
    new_action = climate::CLIMATE_ACTION_OFF;
  } else if (state_.heater_enabled) {
    new_mode = climate::CLIMATE_MODE_HEAT;

    if (state_.heater_red) {
      new_action = climate::CLIMATE_ACTION_HEATING;
    } else {
      new_action = climate::CLIMATE_ACTION_IDLE;
    }
  } else if (state_.filter_pump) {
    new_mode = climate::CLIMATE_MODE_FAN_ONLY;
    new_action = climate::CLIMATE_ACTION_FAN;
  } else {
    new_mode = climate::CLIMATE_MODE_OFF;
    new_action = climate::CLIMATE_ACTION_IDLE;
  }

  if (this->mode != new_mode || this->action != new_action) {
    this->mode = new_mode;
    this->action = new_action;
    state_changed = true;
  }

  // Only publish if something changed
  if (state_changed) {
    this->publish_state();
  }
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
}

// =============================================================================
// CONTROL METHODS
// =============================================================================

void BestwaySpa::set_power(bool state) {
  if (protocol_type_ == PROTOCOL_4WIRE) {
    state_.power = state;
  } else {
    if (state && !state_.power) {
      // Turn on - queue power button
      pending_button_ = POWER;
    } else if (!state && state_.power) {
      // Turn off - queue power button
      pending_button_ = POWER;
    }
  }
  ESP_LOGD(TAG, "Requested power %s", state ? "ON" : "OFF");
}

void BestwaySpa::set_heater(bool state) {
  if (protocol_type_ == PROTOCOL_4WIRE) {
    state_.heater_enabled = state;
    if (state && !state_.filter_pump) {
      state_.filter_pump = true;
    }
  } else {
    if (state != state_.heater_enabled) {
      pending_button_ = HEAT;
    }
  }
  ESP_LOGD(TAG, "Requested heater %s", state ? "ON" : "OFF");
}

void BestwaySpa::set_filter(bool state) {
  if (protocol_type_ == PROTOCOL_4WIRE) {
    state_.filter_pump = state;
    if (!state && state_.heater_enabled) {
      state_.heater_enabled = false;
    }
  } else {
    if (state != state_.filter_pump) {
      pending_button_ = PUMP;
    }
  }
  ESP_LOGD(TAG, "Requested filter %s", state ? "ON" : "OFF");
}

void BestwaySpa::set_bubbles(bool state) {
  if (protocol_type_ == PROTOCOL_4WIRE) {
    state_.bubbles = state;
  } else {
    if (state != state_.bubbles) {
      pending_button_ = BUBBLES;
    }
  }
  ESP_LOGD(TAG, "Requested bubbles %s", state ? "ON" : "OFF");
}

void BestwaySpa::set_jets(bool state) {
  if (!has_jets()) {
    ESP_LOGW(TAG, "This model does not have jets");
    return;
  }

  if (protocol_type_ == PROTOCOL_4WIRE) {
    state_.jets = state;
  } else {
    if (state != state_.jets) {
      pending_button_ = HYDROJETS;
    }
  }
  ESP_LOGD(TAG, "Requested jets %s", state ? "ON" : "OFF");
}

void BestwaySpa::set_lock(bool state) {
  if (protocol_type_ == PROTOCOL_4WIRE) {
    state_.locked = state;
  } else {
    if (state != state_.locked) {
      pending_button_ = LOCK;
    }
  }
  ESP_LOGD(TAG, "Requested lock %s", state ? "ON" : "OFF");
}

void BestwaySpa::set_unit(bool celsius) {
  if (protocol_type_ == PROTOCOL_4WIRE) {
    if (state_.unit_celsius != celsius) {
      state_.unit_celsius = celsius;
      // Convert temperatures
      if (celsius) {
        state_.current_temp = fahrenheit_to_celsius_(state_.current_temp);
        state_.target_temp = fahrenheit_to_celsius_(state_.target_temp);
      } else {
        state_.current_temp = celsius_to_fahrenheit_(state_.current_temp);
        state_.target_temp = celsius_to_fahrenheit_(state_.target_temp);
      }
    }
  } else {
    if (celsius != state_.unit_celsius) {
      pending_button_ = UNIT;
    }
  }
  ESP_LOGD(TAG, "Requested unit %s", celsius ? "C" : "F");
}

void BestwaySpa::set_target_temp(float temp) {
  // Calculate delta from current target
  float delta = temp - state_.target_temp;
  int8_t steps = (int8_t)roundf(delta);

  if (steps != 0) {
    adjust_target_temp(steps);
  }
}

void BestwaySpa::adjust_target_temp(int8_t delta) {
  if (delta == 0) return;

  if (protocol_type_ == PROTOCOL_4WIRE) {
    // For 4-wire, directly adjust target
    state_.target_temp += delta;
    if (state_.unit_celsius) {
      if (state_.target_temp < 20.0f) state_.target_temp = 20.0f;
      if (state_.target_temp > 40.0f) state_.target_temp = 40.0f;
    } else {
      if (state_.target_temp < 68.0f) state_.target_temp = 68.0f;
      if (state_.target_temp > 104.0f) state_.target_temp = 104.0f;
    }
  } else {
    // For 6-wire, queue button presses
    temp_adjust_steps_ += delta;
  }

  ESP_LOGD(TAG, "Adjusting target temperature by %d steps", delta);
}

void BestwaySpa::set_timer(uint8_t hours) {
  ESP_LOGD(TAG, "Setting timer to %d hours", hours);
  if (protocol_type_ != PROTOCOL_4WIRE) {
    pending_button_ = TIMER;
  }
  state_.timer_hours = hours;
}

void BestwaySpa::set_brightness(uint8_t level) {
  if (level > 8) level = 8;
  state_.brightness = level;
  if (protocol_type_ == PROTOCOL_6WIRE_T1) {
    dsp_type1_.get_states().brightness = level;
  }
  ESP_LOGD(TAG, "Set brightness to %d", level);
}

// =============================================================================
// STATE GETTERS
// =============================================================================

bool BestwaySpa::has_jets() const {
  switch (model_) {
    case MODEL_54138:
    case MODEL_54144:
    case MODEL_54173:
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

}  // namespace bestway_spa
}  // namespace esphome
