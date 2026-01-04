#include "bestway_spa.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace bestway_spa {

static const char *const TAG = "bestway_spa";

// =============================================================================
// TIMING CONSTANTS - Based on VisualApproach firmware
// =============================================================================

static const uint32_t PACKET_TIMEOUT_MS = 100;
static const uint32_t STATE_UPDATE_INTERVAL_MS = 500;
static const uint32_t SENSOR_UPDATE_INTERVAL_MS = 2000;
static const uint32_t DSP_REFRESH_INTERVAL_MS = 50;      // ~20Hz display refresh
static const uint32_t BUTTON_POLL_INTERVAL_MS = 100;
static const uint32_t BUTTON_DEBOUNCE_MS = 50;
static const uint32_t CLOCK_PULSE_US = 50;               // Clock pulse width

// 6-wire TYPE1 protocol constants
static const uint8_t DSP_CMD1_MODE6_11_7 = 0x01;
static const uint8_t DSP_CMD1_MODE6_11_7_P05504 = 0x05;
static const uint8_t DSP_CMD2_DATAREAD = 0x42;
static const uint8_t DSP_CMD2_DATAWRITE = 0x40;
static const uint8_t DSP_DIM_BASE = 0x80;
static const uint8_t DSP_DIM_ON = 0x08;

// 6-wire TYPE2 protocol constants
static const uint8_t TYPE2_CMD1 = 0x40;
static const uint8_t TYPE2_CMD2 = 0xC0;
static const uint8_t TYPE2_CMD3 = 0x88;

// Payload byte indices for TYPE1 (11-byte payload)
static const uint8_t T1_DGT1_IDX = 1;
static const uint8_t T1_DGT2_IDX = 3;
static const uint8_t T1_DGT3_IDX = 5;
static const uint8_t T1_TIMER_IDX = 7;
static const uint8_t T1_TIMER_BIT = 1;
static const uint8_t T1_LOCK_IDX = 7;
static const uint8_t T1_LOCK_BIT = 2;
static const uint8_t T1_HEATGRN_IDX = 9;
static const uint8_t T1_HEATGRN_BIT = 0;
static const uint8_t T1_HEATRED_IDX = 9;
static const uint8_t T1_HEATRED_BIT = 3;
static const uint8_t T1_AIR_IDX = 9;
static const uint8_t T1_AIR_BIT = 1;
static const uint8_t T1_FILTER_IDX = 9;
static const uint8_t T1_FILTER_BIT = 2;
static const uint8_t T1_C_IDX = 7;
static const uint8_t T1_C_BIT = 0;
static const uint8_t T1_F_IDX = 9;
static const uint8_t T1_F_BIT = 4;
static const uint8_t T1_POWER_IDX = 9;
static const uint8_t T1_POWER_BIT = 5;
static const uint8_t T1_JETS_IDX = 9;
static const uint8_t T1_JETS_BIT = 6;

// Payload byte indices for TYPE2 (5-byte payload)
static const uint8_t T2_DGT1_IDX = 0;
static const uint8_t T2_DGT2_IDX = 1;
static const uint8_t T2_DGT3_IDX = 2;
static const uint8_t T2_TIMER_IDX = 3;
static const uint8_t T2_TIMER_BIT = 0;
static const uint8_t T2_LOCK_IDX = 3;
static const uint8_t T2_LOCK_BIT = 1;
static const uint8_t T2_HEATGRN_IDX = 3;
static const uint8_t T2_HEATGRN_BIT = 2;
static const uint8_t T2_HEATRED_IDX = 3;
static const uint8_t T2_HEATRED_BIT = 3;
static const uint8_t T2_AIR_IDX = 3;
static const uint8_t T2_AIR_BIT = 4;
static const uint8_t T2_FILTER_IDX = 3;
static const uint8_t T2_FILTER_BIT = 5;
static const uint8_t T2_C_IDX = 3;
static const uint8_t T2_C_BIT = 6;
static const uint8_t T2_F_IDX = 3;
static const uint8_t T2_F_BIT = 7;
static const uint8_t T2_POWER_IDX = 4;
static const uint8_t T2_POWER_BIT = 0;
static const uint8_t T2_JETS_IDX = 4;
static const uint8_t T2_JETS_BIT = 1;

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

  // Initialize 6-wire pins
  if (protocol_type_ == PROTOCOL_6WIRE_T1 || protocol_type_ == PROTOCOL_6WIRE_T2) {
    if (clk_pin_ != nullptr) {
      clk_pin_->setup();
      clk_pin_->digital_write(false);  // Clock idle low
    }
    if (data_pin_ != nullptr) {
      data_pin_->setup();
      data_pin_->pin_mode(gpio::FLAG_OUTPUT);
      data_pin_->digital_write(true);  // Data idle high
    }
    if (cs_pin_ != nullptr) {
      cs_pin_->setup();
      cs_pin_->digital_write(true);    // CS idle high
    }
    if (audio_pin_ != nullptr) {
      audio_pin_->setup();
      audio_pin_->digital_write(false);
    }

    // Initialize default DSP payload
    if (protocol_type_ == PROTOCOL_6WIRE_T1) {
      dsp_payload_len_ = 11;
      dsp_payload_[0] = (model_ == MODEL_P05504) ? DSP_CMD1_MODE6_11_7_P05504 : DSP_CMD1_MODE6_11_7;
      dsp_payload_[1] = 0x00;  // Digit 1
      dsp_payload_[2] = 0x00;
      dsp_payload_[3] = 0x00;  // Digit 2
      dsp_payload_[4] = 0x00;
      dsp_payload_[5] = 0x00;  // Digit 3
      dsp_payload_[6] = 0x00;
      dsp_payload_[7] = 0x00;  // Status byte 1
      dsp_payload_[8] = 0x00;
      dsp_payload_[9] = 0x00;  // Status byte 2
      dsp_payload_[10] = 0x00;
    } else {
      dsp_payload_len_ = 5;
    }
  }

  // Initialize climate state
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

  if (paused_) {
    return;
  }

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

  // Process button queue (for 6-wire)
  if (protocol_type_ != PROTOCOL_4WIRE) {
    process_button_queue_();
  }

  // Handle toggle requests
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
      proto_str = "6-wire TYPE1 (SPI-like)";
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

  if (protocol_type_ != PROTOCOL_4WIRE) {
    if (clk_pin_ != nullptr)
      ESP_LOGCONFIG(TAG, "  CLK Pin: GPIO%d", clk_pin_->get_pin());
    if (data_pin_ != nullptr)
      ESP_LOGCONFIG(TAG, "  DATA Pin: GPIO%d", data_pin_->get_pin());
    if (cs_pin_ != nullptr)
      ESP_LOGCONFIG(TAG, "  CS Pin: GPIO%d", cs_pin_->get_pin());
  }

  LOG_CLIMATE("", "Bestway Spa Climate", this);
}

// =============================================================================
// CLIMATE TRAITS
// =============================================================================

climate::ClimateTraits BestwaySpa::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_supports_two_point_target_temperature(false);

  // Temperature range depends on unit
  if (state_.unit_celsius) {
    traits.set_visual_min_temperature(20.0f);
    traits.set_visual_max_temperature(40.0f);
  } else {
    traits.set_visual_min_temperature(68.0f);
    traits.set_visual_max_temperature(104.0f);
  }

  traits.set_visual_temperature_step(1.0f);
  traits.set_supported_modes({
    climate::CLIMATE_MODE_OFF,
    climate::CLIMATE_MODE_HEAT,
    climate::CLIMATE_MODE_FAN_ONLY
  });

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
        new_packet_available_ = true;
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

  // Send response if we have pending commands
  if (new_packet_available_) {
    send_4wire_response_();
    new_packet_available_ = false;
  }
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
  state_.heater_enabled = state_.heater_red || state_.heater_green;

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
// 6-WIRE TYPE1 PROTOCOL HANDLER
// =============================================================================

void BestwaySpa::handle_6wire_type1_protocol_() {
  uint32_t now = millis();

  // Refresh DSP display at regular intervals
  if (now - last_dsp_refresh_ >= DSP_REFRESH_INTERVAL_MS) {
    send_dsp_payload_type1_();
    last_dsp_refresh_ = now;
  }

  // Poll for button presses at regular intervals
  if (now - last_button_poll_ >= BUTTON_POLL_INTERVAL_MS) {
    receive_cio_payload_type1_();
    update_states_from_payload_();
    last_button_poll_ = now;
  }
}

void BestwaySpa::send_dsp_payload_type1_() {
  if (cs_pin_ == nullptr || clk_pin_ == nullptr || data_pin_ == nullptr) {
    ESP_LOGW(TAG, "6-wire pins not configured");
    return;
  }

  // Set data pin to output mode
  data_pin_->pin_mode(gpio::FLAG_OUTPUT);

  // Start transmission - CS low
  cs_pin_->digital_write(false);
  delayMicroseconds(10);

  // Send 11-byte payload
  for (size_t i = 0; i < 11; i++) {
    send_bits_to_dsp_(dsp_payload_[i], 8);
  }

  // End transmission - CS high
  cs_pin_->digital_write(true);
}

void BestwaySpa::receive_cio_payload_type1_() {
  if (cs_pin_ == nullptr || clk_pin_ == nullptr || data_pin_ == nullptr) {
    return;
  }

  // Send command to request button state
  data_pin_->pin_mode(gpio::FLAG_OUTPUT);

  cs_pin_->digital_write(false);
  delayMicroseconds(10);

  // Send data read command
  uint8_t cmd1 = (model_ == MODEL_P05504) ? DSP_CMD1_MODE6_11_7_P05504 : DSP_CMD1_MODE6_11_7;
  send_bits_to_dsp_(cmd1, 8);
  send_bits_to_dsp_(DSP_CMD2_DATAREAD, 8);

  // Switch to input mode
  data_pin_->pin_mode(gpio::FLAG_INPUT);
  delayMicroseconds(10);

  // Read 16-bit button code
  uint16_t button_code = receive_bits_from_dsp_(16);

  // End transmission
  cs_pin_->digital_write(true);

  // Store button code if valid
  if (button_code != 0xFFFF) {
    current_button_code_ = button_code;
    ESP_LOGV(TAG, "6-wire TYPE1 button code: 0x%04X", button_code);
  }
}

// =============================================================================
// 6-WIRE TYPE2 PROTOCOL HANDLER
// =============================================================================

void BestwaySpa::handle_6wire_type2_protocol_() {
  uint32_t now = millis();

  // Refresh DSP display at regular intervals
  if (now - last_dsp_refresh_ >= DSP_REFRESH_INTERVAL_MS) {
    send_dsp_payload_type2_();
    last_dsp_refresh_ = now;
  }

  // Poll for button presses at regular intervals
  if (now - last_button_poll_ >= BUTTON_POLL_INTERVAL_MS) {
    receive_cio_payload_type2_();
    update_states_from_payload_();
    last_button_poll_ = now;
  }
}

void BestwaySpa::send_dsp_payload_type2_() {
  if (cs_pin_ == nullptr || clk_pin_ == nullptr || data_pin_ == nullptr) {
    ESP_LOGW(TAG, "6-wire pins not configured");
    return;
  }

  // Set data pin to output mode
  data_pin_->pin_mode(gpio::FLAG_OUTPUT);

  // Start transmission - CS low
  cs_pin_->digital_write(false);
  delayMicroseconds(10);

  // Send command byte 1
  send_bits_to_dsp_(TYPE2_CMD1, 8);

  // Send 5-byte payload (LSB first for TYPE2)
  for (size_t i = 0; i < 5; i++) {
    // TYPE2 sends LSB first
    uint8_t byte = dsp_payload_[i];
    for (int bit = 0; bit < 8; bit++) {
      data_pin_->digital_write((byte >> bit) & 0x01);
      pulse_clock_();
    }
  }

  // End transmission - CS high
  cs_pin_->digital_write(true);
  delayMicroseconds(10);

  // Send brightness command
  cs_pin_->digital_write(false);
  send_bits_to_dsp_(TYPE2_CMD3 | (state_.brightness & 0x07), 8);
  cs_pin_->digital_write(true);
}

void BestwaySpa::receive_cio_payload_type2_() {
  if (cs_pin_ == nullptr || clk_pin_ == nullptr || data_pin_ == nullptr) {
    return;
  }

  // Send command to request button state
  data_pin_->pin_mode(gpio::FLAG_OUTPUT);

  cs_pin_->digital_write(false);
  delayMicroseconds(10);

  // Send data read command
  send_bits_to_dsp_(TYPE2_CMD2, 8);

  // Switch to input mode
  data_pin_->pin_mode(gpio::FLAG_INPUT);
  delayMicroseconds(10);

  // Read 16-bit button code (LSB first for TYPE2)
  uint16_t button_code = 0;
  for (int bit = 0; bit < 16; bit++) {
    pulse_clock_();
    if (data_pin_->digital_read()) {
      button_code |= (1 << bit);
    }
  }

  // End transmission
  cs_pin_->digital_write(true);

  // Store button code if valid
  if (button_code != 0x0000) {
    current_button_code_ = button_code;
    ESP_LOGV(TAG, "6-wire TYPE2 button code: 0x%04X", button_code);
  }
}

// =============================================================================
// 6-WIRE BIT-BANGING HELPERS
// =============================================================================

void BestwaySpa::send_bits_to_dsp_(uint32_t data, uint8_t bits) {
  // Send MSB first (TYPE1 style)
  for (int i = bits - 1; i >= 0; i--) {
    data_pin_->digital_write((data >> i) & 0x01);
    pulse_clock_();
  }
}

uint16_t BestwaySpa::receive_bits_from_dsp_(uint8_t bits) {
  uint16_t result = 0;

  // Read MSB first (TYPE1 style)
  for (int i = bits - 1; i >= 0; i--) {
    pulse_clock_();
    if (data_pin_->digital_read()) {
      result |= (1 << i);
    }
  }

  return result;
}

void BestwaySpa::pulse_clock_(uint32_t duration_us) {
  clk_pin_->digital_write(true);
  delayMicroseconds(duration_us);
  clk_pin_->digital_write(false);
  delayMicroseconds(duration_us);
}

// =============================================================================
// STATE MANAGEMENT
// =============================================================================

void BestwaySpa::update_states_from_payload_() {
  // Decode button press from current_button_code_
  const uint16_t *btn_codes;
  switch (model_) {
    case MODEL_PRE2021:
      btn_codes = BTN_CODES_PRE2021;
      break;
    case MODEL_P05504:
      btn_codes = BTN_CODES_P05504;
      break;
    case MODEL_54149E:
      btn_codes = BTN_CODES_54149E;
      break;
    default:
      btn_codes = BTN_CODES_PRE2021;
      break;
  }

  // Find which button was pressed
  for (uint8_t i = 0; i < BTN_COUNT; i++) {
    if (current_button_code_ == btn_codes[i] && i != NOBTN) {
      ESP_LOGD(TAG, "Button pressed: %d", i);
      switch (i) {
        case LOCK:
          state_.locked = !state_.locked;
          break;
        case POWER:
          state_.power = !state_.power;
          break;
        case HEAT:
          if (!state_.locked && state_.power) {
            state_.heater_enabled = !state_.heater_enabled;
          }
          break;
        case PUMP:
          if (!state_.locked && state_.power) {
            state_.filter_pump = !state_.filter_pump;
          }
          break;
        case BUBBLES:
          if (!state_.locked && state_.power) {
            state_.bubbles = !state_.bubbles;
          }
          break;
        case HYDROJETS:
          if (!state_.locked && state_.power && has_jets()) {
            state_.jets = !state_.jets;
          }
          break;
        case UP:
          if (!state_.locked && state_.power) {
            state_.target_temp += 1.0f;
            if (state_.unit_celsius && state_.target_temp > 40.0f)
              state_.target_temp = 40.0f;
            if (!state_.unit_celsius && state_.target_temp > 104.0f)
              state_.target_temp = 104.0f;
          }
          break;
        case DOWN:
          if (!state_.locked && state_.power) {
            state_.target_temp -= 1.0f;
            if (state_.unit_celsius && state_.target_temp < 20.0f)
              state_.target_temp = 20.0f;
            if (!state_.unit_celsius && state_.target_temp < 68.0f)
              state_.target_temp = 68.0f;
          }
          break;
        case UNIT:
          if (!state_.locked && state_.power) {
            state_.unit_celsius = !state_.unit_celsius;
            // Convert temperatures
            if (state_.unit_celsius) {
              state_.current_temp = fahrenheit_to_celsius_(state_.current_temp);
              state_.target_temp = fahrenheit_to_celsius_(state_.target_temp);
            } else {
              state_.current_temp = celsius_to_fahrenheit_(state_.current_temp);
              state_.target_temp = celsius_to_fahrenheit_(state_.target_temp);
            }
          }
          break;
        case TIMER:
          if (!state_.locked && state_.power) {
            state_.timer_active = !state_.timer_active;
          }
          break;
      }
      break;
    }
  }

  // Reset button code
  current_button_code_ = btn_codes[NOBTN];
}

void BestwaySpa::handle_toggles_() {
  // Process toggle requests from Home Assistant / automation

  if (toggles_.power_pressed) {
    queue_button_(POWER, 300);
    toggles_.power_pressed = false;
  }

  if (toggles_.lock_pressed) {
    queue_button_(LOCK, 300);
    toggles_.lock_pressed = false;
  }

  if (toggles_.heat_pressed) {
    queue_button_(HEAT, 300);
    toggles_.heat_pressed = false;
  }

  if (toggles_.pump_pressed) {
    queue_button_(PUMP, 300);
    toggles_.pump_pressed = false;
  }

  if (toggles_.bubbles_pressed) {
    queue_button_(BUBBLES, 300);
    toggles_.bubbles_pressed = false;
  }

  if (toggles_.jets_pressed && has_jets()) {
    queue_button_(HYDROJETS, 300);
    toggles_.jets_pressed = false;
  }

  if (toggles_.unit_pressed) {
    queue_button_(UNIT, 300);
    toggles_.unit_pressed = false;
  }

  if (toggles_.timer_pressed) {
    queue_button_(TIMER, 300);
    toggles_.timer_pressed = false;
  }

  // Handle temperature adjustment
  if (toggles_.set_target_temp && toggles_.target_temp_delta != 0) {
    Buttons btn = toggles_.target_temp_delta > 0 ? UP : DOWN;
    int steps = abs(toggles_.target_temp_delta);
    for (int i = 0; i < steps; i++) {
      queue_button_(btn, 300);
    }
    toggles_.target_temp_delta = 0;
    toggles_.set_target_temp = false;
  }
}

void BestwaySpa::update_climate_state_() {
  // Update current temperature
  this->current_temperature = state_.current_temp;
  this->target_temperature = state_.target_temp;

  // Determine mode based on state
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
}

// =============================================================================
// BUTTON QUEUE FOR 6-WIRE
// =============================================================================

void BestwaySpa::queue_button_(Buttons button, int duration_ms) {
  if (!button_enabled_[button]) {
    ESP_LOGD(TAG, "Button %d is disabled", button);
    return;
  }

  ButtonQueueItem item;
  item.button_code = get_button_code_(button);
  item.duration_ms = duration_ms;
  item.start_time = 0;  // Will be set when processing starts
  item.target_state = 0xFF;  // Don't wait for state change
  item.target_value = 0;

  button_queue_.push_back(item);
  ESP_LOGD(TAG, "Queued button %d (code 0x%04X) for %dms", button, item.button_code, duration_ms);
}

void BestwaySpa::process_button_queue_() {
  if (button_queue_.empty()) {
    current_button_code_ = get_button_code_(NOBTN);
    return;
  }

  uint32_t now = millis();
  auto &item = button_queue_.front();

  if (item.start_time == 0) {
    // Start pressing this button
    item.start_time = now;
    current_button_code_ = item.button_code;
    ESP_LOGV(TAG, "Started pressing button 0x%04X", item.button_code);
  }

  // Check if button press duration has elapsed
  if ((now - item.start_time) >= (uint32_t)item.duration_ms) {
    ESP_LOGV(TAG, "Finished pressing button 0x%04X", item.button_code);
    button_queue_.erase(button_queue_.begin());
  }
}

uint16_t BestwaySpa::get_button_code_(Buttons button) {
  if (button >= BTN_COUNT) {
    return 0x1B1B;
  }

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

// =============================================================================
// CHARACTER DECODING
// =============================================================================

char BestwaySpa::decode_7segment_(uint8_t segments, bool is_type1) {
  const uint8_t *codes = is_type1 ? CHARCODES_TYPE1 : CHARCODES_TYPE2;
  size_t code_count = sizeof(CHARCODES_TYPE1) / sizeof(CHARCODES_TYPE1[0]);

  for (size_t i = 0; i < code_count; i++) {
    if (codes[i] == segments) {
      return CHARS[i];
    }
  }
  return '?';  // Unknown segment pattern
}

// =============================================================================
// CONTROL METHODS
// =============================================================================

void BestwaySpa::set_power(bool state) {
  if (state_.power != state) {
    if (protocol_type_ == PROTOCOL_4WIRE) {
      // 4-wire doesn't have power button - toggle via heater/pump
      state_.power = state;
    } else {
      toggles_.power_pressed = true;
    }
    ESP_LOGD(TAG, "Requested power %s", state ? "ON" : "OFF");
  }
}

void BestwaySpa::set_heater(bool state) {
  if (state_.heater_enabled != state) {
    if (protocol_type_ == PROTOCOL_4WIRE) {
      state_.heater_enabled = state;
      // For 4-wire, filter must be on for heater
      if (state && !state_.filter_pump) {
        state_.filter_pump = true;
      }
    } else {
      toggles_.heat_pressed = true;
    }
    ESP_LOGD(TAG, "Requested heater %s", state ? "ON" : "OFF");
  }
}

void BestwaySpa::set_filter(bool state) {
  if (state_.filter_pump != state) {
    if (protocol_type_ == PROTOCOL_4WIRE) {
      state_.filter_pump = state;
      // For 4-wire, turning off filter turns off heater
      if (!state && state_.heater_enabled) {
        state_.heater_enabled = false;
      }
    } else {
      toggles_.pump_pressed = true;
    }
    ESP_LOGD(TAG, "Requested filter %s", state ? "ON" : "OFF");
  }
}

void BestwaySpa::set_bubbles(bool state) {
  if (state_.bubbles != state) {
    if (protocol_type_ == PROTOCOL_4WIRE) {
      state_.bubbles = state;
    } else {
      toggles_.bubbles_pressed = true;
    }
    ESP_LOGD(TAG, "Requested bubbles %s", state ? "ON" : "OFF");
  }
}

void BestwaySpa::set_jets(bool state) {
  if (!has_jets()) {
    ESP_LOGW(TAG, "This model does not have jets");
    return;
  }

  if (state_.jets != state) {
    if (protocol_type_ == PROTOCOL_4WIRE) {
      state_.jets = state;
    } else {
      toggles_.jets_pressed = true;
    }
    ESP_LOGD(TAG, "Requested jets %s", state ? "ON" : "OFF");
  }
}

void BestwaySpa::set_lock(bool state) {
  if (state_.locked != state) {
    if (protocol_type_ == PROTOCOL_4WIRE) {
      state_.locked = state;
    } else {
      toggles_.lock_pressed = true;
    }
    ESP_LOGD(TAG, "Requested lock %s", state ? "ON" : "OFF");
  }
}

void BestwaySpa::set_unit(bool celsius) {
  if (state_.unit_celsius != celsius) {
    if (protocol_type_ == PROTOCOL_4WIRE) {
      state_.unit_celsius = celsius;
      // Convert temperatures
      if (celsius) {
        state_.current_temp = fahrenheit_to_celsius_(state_.current_temp);
        state_.target_temp = fahrenheit_to_celsius_(state_.target_temp);
      } else {
        state_.current_temp = celsius_to_fahrenheit_(state_.current_temp);
        state_.target_temp = celsius_to_fahrenheit_(state_.target_temp);
      }
    } else {
      toggles_.unit_pressed = true;
    }
    ESP_LOGD(TAG, "Requested unit %s", celsius ? "C" : "F");
  }
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
    toggles_.set_target_temp = true;
    toggles_.target_temp_delta = delta;
  }

  ESP_LOGD(TAG, "Adjusting target temperature by %d steps", delta);
}

void BestwaySpa::set_timer(uint8_t hours) {
  ESP_LOGD(TAG, "Setting timer to %d hours", hours);
  // Timer is typically just toggle on 6-wire
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
