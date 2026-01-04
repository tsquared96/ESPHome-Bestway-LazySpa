#include "bestway_spa.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <Arduino.h>

namespace esphome {
namespace bestway_spa {

static const char *const TAG = "bestway_spa";

// =============================================================================
// TIMING CONSTANTS
// =============================================================================

static const uint32_t STATE_UPDATE_INTERVAL_MS = 500;
static const uint32_t SENSOR_UPDATE_INTERVAL_MS = 2000;
static const uint32_t BUTTON_HOLD_MS = 300;        // How long to hold a button press
static const uint32_t STATS_INTERVAL_MS = 5000;    // Statistics logging interval

// 4-wire protocol constants
static const uint32_t PACKET_TIMEOUT_MS = 100;

// =============================================================================
// STATIC/GLOBAL FOR ISR ACCESS
// =============================================================================

// ISR needs access to the instance - we support only one spa per ESP
static BestwaySpa *g_spa_instance = nullptr;

// Pin numbers for direct GPIO access in ISR
static volatile int g_data_pin = -1;
static volatile int g_clk_pin = -1;
static volatile int g_cs_pin = -1;

// Volatile variables accessed from ISR
static volatile uint8_t g_cio_buffer[16];      // Buffer for CIO data
static volatile uint8_t g_cio_bit_count = 0;   // Bit counter within byte
static volatile uint8_t g_cio_byte_count = 0;  // Byte counter within packet
static volatile bool g_cio_packet_ready = false;
static volatile uint16_t g_dsp_button_code = 0x1B1B;  // Button code to send (NOBTN)
static volatile uint8_t g_dsp_bit_count = 0;
static volatile uint32_t g_cs_count = 0;       // CS interrupt counter
static volatile uint32_t g_clk_count = 0;      // CLK interrupt counter
static volatile uint32_t g_bad_packets = 0;    // Bad packet counter

// =============================================================================
// INTERRUPT SERVICE ROUTINES FOR 6-WIRE (Arduino native)
// =============================================================================

// CS (Chip Select) change - handles both falling (start) and rising (end) edges
void IRAM_ATTR cio_cs_change_isr() {
  if (g_cs_pin < 0) return;

  bool cs_high = digitalRead(g_cs_pin);
  g_cs_count++;

  if (!cs_high) {
    // CS went LOW - falling edge - start of packet
    g_cio_bit_count = 0;
    g_cio_byte_count = 0;
    g_dsp_bit_count = 0;

    // Clear buffer
    for (int i = 0; i < 11; i++) {
      g_cio_buffer[i] = 0;
    }
  } else {
    // CS went HIGH - rising edge - end of packet
    // Validate packet length (should be 11 bytes = 88 bits for TYPE1)
    if (g_cio_byte_count >= 11 || (g_cio_byte_count == 10 && g_cio_bit_count > 0)) {
      g_cio_packet_ready = true;
    } else {
      g_bad_packets++;
    }
  }
}

// CLK rising edge - sample data bit and output button bit
void IRAM_ATTR cio_clk_rising_isr() {
  if (g_data_pin < 0) return;

  g_clk_count++;

  // Read bit from CIO (DATA line) - direct GPIO read
  bool bit_value = digitalRead(g_data_pin);

  // Store bit in buffer (MSB first)
  if (g_cio_byte_count < 11) {
    g_cio_buffer[g_cio_byte_count] = (g_cio_buffer[g_cio_byte_count] << 1) | (bit_value ? 1 : 0);
    g_cio_bit_count++;

    if (g_cio_bit_count >= 8) {
      g_cio_bit_count = 0;
      g_cio_byte_count++;
    }
  }

  // Output button code bit (MSB first, 16 bits)
  if (g_dsp_bit_count < 16) {
    int bit_pos = 15 - g_dsp_bit_count;
    bool out_bit = (g_dsp_button_code >> bit_pos) & 0x01;
    // Direct GPIO write for speed
    digitalWrite(g_data_pin, out_bit);
    g_dsp_bit_count++;
  }
}

// =============================================================================
// SETUP
// =============================================================================

void BestwaySpa::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Bestway Spa...");

  // Set global instance for ISR access
  g_spa_instance = this;

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

  // Initialize 6-wire pins with interrupt-driven I/O using Arduino native functions
  if (protocol_type_ == PROTOCOL_6WIRE_T1 || protocol_type_ == PROTOCOL_6WIRE_T2) {
    // Get pin numbers for direct GPIO access in ISR
    if (clk_pin_ != nullptr) {
      g_clk_pin = clk_pin_->get_pin();
      pinMode(g_clk_pin, INPUT);
      attachInterrupt(digitalPinToInterrupt(g_clk_pin), cio_clk_rising_isr, RISING);
      ESP_LOGD(TAG, "CLK interrupt attached on GPIO%d (Arduino native)", g_clk_pin);
    }

    if (data_pin_ != nullptr) {
      g_data_pin = data_pin_->get_pin();
      pinMode(g_data_pin, INPUT);  // Start as input, toggle as needed
      ESP_LOGD(TAG, "DATA pin configured on GPIO%d", g_data_pin);
    }

    if (cs_pin_ != nullptr) {
      g_cs_pin = cs_pin_->get_pin();
      pinMode(g_cs_pin, INPUT);
      // Use CHANGE to detect both falling (start) and rising (end) edges
      attachInterrupt(digitalPinToInterrupt(g_cs_pin), cio_cs_change_isr, CHANGE);
      ESP_LOGD(TAG, "CS interrupt attached on GPIO%d with CHANGE mode", g_cs_pin);
    }

    // Audio pin (optional) - OUTPUT
    if (audio_pin_ != nullptr) {
      int audio_gpio = audio_pin_->get_pin();
      pinMode(audio_gpio, OUTPUT);
      digitalWrite(audio_gpio, LOW);
      ESP_LOGD(TAG, "Audio pin configured on GPIO%d", audio_gpio);
    }

    // Initialize button code to NOBTN
    g_dsp_button_code = get_button_code_(NOBTN);

    ESP_LOGI(TAG, "6-wire protocol initialized with Arduino native interrupts");
    ESP_LOGI(TAG, "  CLK=%d, DATA=%d, CS=%d", g_clk_pin, g_data_pin, g_cs_pin);
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

  // Process button queue and update DSP output
  if (protocol_type_ != PROTOCOL_4WIRE) {
    process_button_queue_();

    // Update the button code that ISR sends
    g_dsp_button_code = current_button_code_;
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
    uint32_t cs_delta = g_cs_count - last_cs_count_;
    uint32_t clk_delta = g_clk_count - last_clk_count_;
    uint32_t pkt_delta = good_packets_ - last_pkt_count_;

    ESP_LOGI(TAG, "CIO: pkts=%lu(+%lu) bad=%lu | ISR: cs=%lu(+%lu) clk=%lu(+%lu) | Btn:0x%04X",
             (unsigned long)good_packets_, (unsigned long)pkt_delta,
             (unsigned long)g_bad_packets,
             (unsigned long)g_cs_count, (unsigned long)cs_delta,
             (unsigned long)g_clk_count, (unsigned long)clk_delta,
             current_button_code_);

    last_cs_count_ = g_cs_count;
    last_clk_count_ = g_clk_count;
    last_pkt_count_ = good_packets_;
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
      proto_str = "6-wire TYPE1 (SPI-like, interrupt-driven)";
      break;
    case PROTOCOL_6WIRE_T2:
      proto_str = "6-wire TYPE2 (SPI-like, interrupt-driven)";
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
      ESP_LOGCONFIG(TAG, "  CIO CLK Pin: GPIO%d", clk_pin_->get_pin());
    if (data_pin_ != nullptr)
      ESP_LOGCONFIG(TAG, "  CIO DATA Pin: GPIO%d", data_pin_->get_pin());
    if (cs_pin_ != nullptr)
      ESP_LOGCONFIG(TAG, "  CIO CS Pin: GPIO%d", cs_pin_->get_pin());
    ESP_LOGCONFIG(TAG, "  CIO Good packets: %lu", (unsigned long)good_packets_);
    ESP_LOGCONFIG(TAG, "  CIO Bad packets: %lu", (unsigned long)g_bad_packets);
  }

  LOG_CLIMATE("", "Bestway Spa Climate", this);
}

// =============================================================================
// CLIMATE TRAITS
// =============================================================================

climate::ClimateTraits BestwaySpa::traits() {
  auto traits = climate::ClimateTraits();

  // Use feature flags for modern ESPHome API
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
// 6-WIRE INTERRUPT-DRIVEN PROTOCOL HANDLER
// =============================================================================

void BestwaySpa::handle_6wire_protocol_() {
  // Check if a new packet is ready from ISR
  if (g_cio_packet_ready) {
    // Copy volatile buffer to local
    uint8_t packet[11];
    noInterrupts();
    for (int i = 0; i < 11; i++) {
      packet[i] = g_cio_buffer[i];
    }
    g_cio_packet_ready = false;
    interrupts();

    // Process the packet
    parse_6wire_cio_packet_(packet);
    good_packets_++;
  }
}

void BestwaySpa::parse_6wire_cio_packet_(const uint8_t *packet) {
  // Log raw packet for debugging
  ESP_LOGD(TAG, "CIO Raw: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
           packet[0], packet[1], packet[2], packet[3], packet[4],
           packet[5], packet[6], packet[7], packet[8], packet[9], packet[10]);

  // Decode 7-segment display digits
  // Bytes 1-2: digit 1, Bytes 3-4: digit 2, Bytes 5-6: digit 3
  char c1 = decode_7segment_(packet[1], true);
  char c2 = decode_7segment_(packet[3], true);
  char c3 = decode_7segment_(packet[5], true);

  state_.display_chars[0] = c1;
  state_.display_chars[1] = c2;
  state_.display_chars[2] = c3;
  state_.display_chars[3] = '\0';

  // Parse temperature from display (if showing temperature)
  if (c1 >= '0' && c1 <= '9') {
    state_.current_temp = (c1 - '0') * 10 + (c2 >= '0' && c2 <= '9' ? (c2 - '0') : 0);
  } else if (c2 >= '0' && c2 <= '9' && c3 >= '0' && c3 <= '9') {
    state_.current_temp = (c2 - '0') * 10 + (c3 - '0');
  }

  // Parse status byte 7 (timer, lock, unit)
  uint8_t status1 = packet[7];
  state_.timer_active = (status1 & 0x02) != 0;  // Timer bit
  state_.locked = (status1 & 0x04) != 0;        // Lock bit
  state_.unit_celsius = (status1 & 0x01) == 0;  // C/F bit (0 = C)

  // Parse status byte 9 (heat, air, filter, power, jets)
  uint8_t status2 = packet[9];
  state_.heater_green = (status2 & 0x01) != 0;  // Heat standby
  state_.bubbles = (status2 & 0x02) != 0;       // Air bubbles
  state_.filter_pump = (status2 & 0x04) != 0;   // Filter pump
  state_.heater_red = (status2 & 0x08) != 0;    // Heating active
  state_.power = (status2 & 0x20) != 0;         // Power on
  state_.jets = (status2 & 0x40) != 0;          // Jets (if available)

  state_.heater_enabled = state_.heater_green || state_.heater_red;

  ESP_LOGD(TAG, "CIO Chars: '%c' '%c' '%c' Temp:%.0f Power:%d Heat:%d Pump:%d",
           c1, c2, c3, state_.current_temp,
           state_.power ? 1 : 0,
           state_.heater_enabled ? 1 : 0,
           state_.filter_pump ? 1 : 0);
}

// =============================================================================
// DATA PIN HELPERS FOR ISR
// =============================================================================

bool BestwaySpa::read_data_pin_() {
  if (data_pin_ != nullptr) {
    return data_pin_->digital_read();
  }
  return false;
}

void BestwaySpa::write_data_pin_(bool value) {
  if (data_pin_ != nullptr) {
    // Briefly switch to output, write, switch back to input
    // This allows bidirectional communication on single wire
    data_pin_->pin_mode(gpio::FLAG_OUTPUT);
    data_pin_->digital_write(value);
    // Note: We keep it in output mode during the transmission cycle
    // The ISR will handle this during the clock pulses
  }
}

// =============================================================================
// STATE MANAGEMENT
// =============================================================================

void BestwaySpa::handle_toggles_() {
  // Process toggle requests from Home Assistant / automation

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

  // Handle temperature adjustment
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
    ESP_LOGI(TAG, "Pressing button 0x%04X", item.button_code);
  }

  // Check if button press duration has elapsed
  if ((now - item.start_time) >= (uint32_t)item.duration_ms) {
    ESP_LOGD(TAG, "Released button 0x%04X", item.button_code);
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

// 7-segment character codes for TYPE1 (matching VisualApproach)
static const uint8_t SEGMENT_CODES[] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F,  // 0-9
  0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71,  // A-F
  0x00, 0x40  // space, dash
};
static const char SEGMENT_CHARS[] = "0123456789ABCDEF -";

char BestwaySpa::decode_7segment_(uint8_t segments, bool is_type1) {
  // For TYPE1, segments may be inverted or shifted
  // Try direct match first
  for (size_t i = 0; i < sizeof(SEGMENT_CODES); i++) {
    if (SEGMENT_CODES[i] == segments) {
      return SEGMENT_CHARS[i];
    }
    // Also try inverted
    if (SEGMENT_CODES[i] == (uint8_t)(~segments)) {
      return SEGMENT_CHARS[i];
    }
  }

  // Special case for common patterns
  if (segments == 0x00 || segments == 0xFF) {
    return ' ';
  }

  return '?';  // Unknown segment pattern
}

// =============================================================================
// CONTROL METHODS
// =============================================================================

void BestwaySpa::set_power(bool state) {
  if (state_.power != state) {
    if (protocol_type_ == PROTOCOL_4WIRE) {
      state_.power = state;
    } else {
      toggles_.power_pressed = true;
      ESP_LOGD(TAG, "Requested power %s", state ? "ON" : "OFF");
    }
  }
}

void BestwaySpa::set_heater(bool state) {
  if (state_.heater_enabled != state) {
    if (protocol_type_ == PROTOCOL_4WIRE) {
      state_.heater_enabled = state;
      if (state && !state_.filter_pump) {
        state_.filter_pump = true;
      }
    } else {
      toggles_.heat_pressed = true;
      ESP_LOGD(TAG, "Requested heater %s", state ? "ON" : "OFF");
    }
  }
}

void BestwaySpa::set_filter(bool state) {
  if (state_.filter_pump != state) {
    if (protocol_type_ == PROTOCOL_4WIRE) {
      state_.filter_pump = state;
      if (!state && state_.heater_enabled) {
        state_.heater_enabled = false;
      }
    } else {
      toggles_.pump_pressed = true;
      ESP_LOGD(TAG, "Requested filter %s", state ? "ON" : "OFF");
    }
  }
}

void BestwaySpa::set_bubbles(bool state) {
  if (state_.bubbles != state) {
    if (protocol_type_ == PROTOCOL_4WIRE) {
      state_.bubbles = state;
    } else {
      toggles_.bubbles_pressed = true;
      ESP_LOGD(TAG, "Requested bubbles %s", state ? "ON" : "OFF");
    }
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
      ESP_LOGD(TAG, "Requested jets %s", state ? "ON" : "OFF");
    }
  }
}

void BestwaySpa::set_lock(bool state) {
  if (state_.locked != state) {
    if (protocol_type_ == PROTOCOL_4WIRE) {
      state_.locked = state;
    } else {
      toggles_.lock_pressed = true;
      ESP_LOGD(TAG, "Requested lock %s", state ? "ON" : "OFF");
    }
  }
}

void BestwaySpa::set_unit(bool celsius) {
  if (state_.unit_celsius != celsius) {
    if (protocol_type_ == PROTOCOL_4WIRE) {
      state_.unit_celsius = celsius;
      if (celsius) {
        state_.current_temp = fahrenheit_to_celsius_(state_.current_temp);
        state_.target_temp = fahrenheit_to_celsius_(state_.target_temp);
      } else {
        state_.current_temp = celsius_to_fahrenheit_(state_.current_temp);
        state_.target_temp = celsius_to_fahrenheit_(state_.target_temp);
      }
    } else {
      toggles_.unit_pressed = true;
      ESP_LOGD(TAG, "Requested unit %s", celsius ? "C" : "F");
    }
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
