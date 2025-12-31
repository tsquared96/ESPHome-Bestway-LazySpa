#include "bestway_spa.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace bestway_spa {

static const char *const TAG = "bestway_spa";

// Protocol timing constants (from Visualapproach)
static const uint32_t PACKET_TIMEOUT_MS = 100;
static const uint32_t RESPONSE_TIMEOUT_MS = 50;
static const uint32_t STATE_UPDATE_INTERVAL_MS = 500;
static const uint32_t SENSOR_UPDATE_INTERVAL_MS = 2000;

void BestwaySpa::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Bestway Spa...");
  
  // Initialize climate state
  this->mode = climate::CLIMATE_MODE_HEAT;
  this->action = climate::CLIMATE_ACTION_IDLE;
  this->current_temperature = state_.current_temp;
  this->target_temperature = state_.target_temp;
  
  ESP_LOGCONFIG(TAG, "Bestway Spa initialized (Protocol: %s)", 
                protocol_type_ == PROTOCOL_4WIRE ? "4-wire" : "6-wire");
}

void BestwaySpa::loop() {
  const uint32_t now = millis();
  
  // Handle protocol based on type
  if (protocol_type_ == PROTOCOL_4WIRE) {
    handle_4wire_protocol_();
  } else {
    handle_6wire_protocol_();
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
  
  // Process command queue
  if (!command_queue_.empty() && !waiting_for_response_) {
    auto &cmd = command_queue_.front();
    if (!cmd.processed && (now - cmd.time) >= 100) {  // Debounce
      send_button_press_(cmd.button);
      cmd.processed = true;
      waiting_for_response_ = true;
      command_queue_.erase(command_queue_.begin());
    }
  }
  
  // Reset waiting flag if timeout
  if (waiting_for_response_ && (now - last_packet_time_) > RESPONSE_TIMEOUT_MS) {
    waiting_for_response_ = false;
  }
}

void BestwaySpa::dump_config() {
  ESP_LOGCONFIG(TAG, "Bestway Spa:");
  ESP_LOGCONFIG(TAG, "  Protocol: %s", protocol_type_ == PROTOCOL_4WIRE ? "4-wire UART" : "6-wire SPI-like");
  LOG_CLIMATE("", "Bestway Spa Climate", this);
}

climate::ClimateTraits BestwaySpa::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_supports_two_point_target_temperature(false);
  
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
    float delta = new_target - state_.target_temp;
    
    // Send button presses to adjust temperature
    int8_t steps = (int8_t)roundf(delta);
    if (steps != 0) {
      adjust_target_temp(steps);
    }
  }
}

// 4-Wire UART Protocol Handler
void BestwaySpa::handle_4wire_protocol_() {
  // Read available bytes from UART
  while (available()) {
    uint8_t byte;
    read_byte(&byte);
    rx_buffer_.push_back(byte);
    last_packet_time_ = millis();
  }
  
  // Process complete packets
  if (rx_buffer_.size() >= 2) {
    const uint32_t now = millis();
    
    // Check if we have a query packet (0x1B 0x1B = no button pressed)
    if (rx_buffer_.size() == 2 && 
        rx_buffer_[0] == 0x1B && rx_buffer_[1] == 0x1B) {
      // CIO is asking DSP which button is pressed
      parse_cio_to_dsp_packet_(rx_buffer_);
      rx_buffer_.clear();
    }
    // Button code packet (0x10 0xXX)
    else if (rx_buffer_.size() == 2 && rx_buffer_[0] == 0x10) {
      parse_cio_to_dsp_packet_(rx_buffer_);
      rx_buffer_.clear();
    }
    // Display segment data (longer packets)
    else if (rx_buffer_.size() > 2 && (now - last_packet_time_) > PACKET_TIMEOUT_MS) {
      parse_cio_to_dsp_packet_(rx_buffer_);
      rx_buffer_.clear();
    }
  }
  
  // Clear buffer if timeout
  if (!rx_buffer_.empty() && (millis() - last_packet_time_) > PACKET_TIMEOUT_MS) {
    ESP_LOGV(TAG, "Packet timeout, clearing buffer");
    rx_buffer_.clear();
  }
}

// 6-Wire Protocol Handler (placeholder for future implementation)
void BestwaySpa::handle_6wire_protocol_() {
  ESP_LOGW(TAG, "6-wire protocol not yet implemented");
  // TODO: Implement 6-wire SPI-like protocol
}

// Parse packets from CIO to DSP
void BestwaySpa::parse_cio_to_dsp_packet_(const std::vector<uint8_t> &packet) {
  if (packet.size() < 2) return;
  
  // Query packet - CIO asking DSP for button state
  if (packet[0] == 0x1B && packet[1] == 0x1B) {
    ESP_LOGV(TAG, "CIO query: no button pressed");
    
    // If we have a pending command, respond with button code
    // Otherwise forward the query
    forward_packet_(packet);
    return;
  }
  
  // Button acknowledgment packet
  if (packet[0] == 0x10) {
    uint8_t button_code = packet[1];
    ESP_LOGD(TAG, "Button code: 0x%02X", button_code);
    
    // Update state based on button
    switch (button_code) {
      case 0x12:  // Heater
        state_.heater = !state_.heater;
        break;
      case 0x13:  // Filter
        state_.filter = !state_.filter;
        break;
      case 0x14:  // Bubbles
        state_.bubbles = !state_.bubbles;
        break;
      case 0x15:  // Temp up
        state_.target_temp += 1.0f;
        if (!state_.unit_celsius && state_.target_temp > 104.0f) state_.target_temp = 104.0f;
        if (state_.unit_celsius && state_.target_temp > 40.0f) state_.target_temp = 40.0f;
        break;
      case 0x16:  // Temp down
        state_.target_temp -= 1.0f;
        if (!state_.unit_celsius && state_.target_temp < 68.0f) state_.target_temp = 68.0f;
        if (state_.unit_celsius && state_.target_temp < 20.0f) state_.target_temp = 20.0f;
        break;
      case 0x17:  // Lock
        state_.locked = !state_.locked;
        break;
      case 0x18:  // Unit toggle
        state_.unit_celsius = !state_.unit_celsius;
        // Convert temperatures
        if (state_.unit_celsius) {
          state_.current_temp = fahrenheit_to_celsius_(state_.current_temp);
          state_.target_temp = fahrenheit_to_celsius_(state_.target_temp);
        } else {
          state_.current_temp = celsius_to_fahrenheit_(state_.current_temp);
          state_.target_temp = celsius_to_fahrenheit_(state_.target_temp);
        }
        break;
    }
    
    forward_packet_(packet);
    return;
  }
  
  // Display segment data - extract temperature and LED states
  if (packet.size() > 4) {
    ESP_LOGV(TAG, "Display segment data, %d bytes", packet.size());
    
    // Parse LED states from segment data
    // This is a simplified version - actual parsing depends on packet structure
    // Based on Visualapproach code, LED states are encoded in the segment data
    
    // For now, just forward the packet
    forward_packet_(packet);
  }
}

// Parse packets from DSP to CIO (button presses from physical panel)
void BestwaySpa::parse_dsp_to_cio_packet_(const std::vector<uint8_t> &packet) {
  // This would handle responses from the display panel
  // For now, we mainly inject button presses, so this is less critical
  forward_packet_(packet);
}

// Send a button press to the CIO
void BestwaySpa::send_button_press_(ButtonCode button) {
  uint8_t packet[2];
  packet[0] = (button >> 8) & 0xFF;
  packet[1] = button & 0xFF;
  
  ESP_LOGD(TAG, "Sending button: 0x%04X", button);
  write_array(packet, 2);
  flush();
  
  last_button_code_ = button;
}

// Forward packet through (man-in-the-middle passthrough)
void BestwaySpa::forward_packet_(const std::vector<uint8_t> &packet) {
  if (!packet.empty()) {
    write_array(packet.data(), packet.size());
    flush();
  }
}

// Update climate state from spa state
void BestwaySpa::update_climate_state_() {
  // Update current temperature
  this->current_temperature = state_.current_temp;
  this->target_temperature = state_.target_temp;
  
  // Determine mode based on heater and filter state
  if (!state_.heater && !state_.filter) {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->action = climate::CLIMATE_ACTION_OFF;
  } else if (state_.heater) {
    this->mode = climate::CLIMATE_MODE_HEAT;
    
    if (state_.heating_active) {
      this->action = climate::CLIMATE_ACTION_HEATING;
    } else {
      this->action = climate::CLIMATE_ACTION_IDLE;
    }
  } else if (state_.filter) {
    this->mode = climate::CLIMATE_MODE_FAN_ONLY;
    this->action = climate::CLIMATE_ACTION_FAN;
  }
  
  this->publish_state();
}

// Update all sensors
void BestwaySpa::update_sensors_() {
  if (current_temp_sensor_ != nullptr) {
    current_temp_sensor_->publish_state(state_.current_temp);
  }
  
  if (target_temp_sensor_ != nullptr) {
    target_temp_sensor_->publish_state(state_.target_temp);
  }
  
  if (heating_sensor_ != nullptr) {
    heating_sensor_->publish_state(state_.heating_active);
  }
  
  if (filter_sensor_ != nullptr) {
    filter_sensor_->publish_state(state_.filter);
  }
  
  if (bubbles_sensor_ != nullptr) {
    bubbles_sensor_->publish_state(state_.bubbles);
  }
  
  if (locked_sensor_ != nullptr) {
    locked_sensor_->publish_state(state_.locked);
  }
  
  if (error_sensor_ != nullptr) {
    error_sensor_->publish_state(state_.error_code != 0);
  }
  
  if (error_text_sensor_ != nullptr && state_.error_code != 0) {
    char error_str[8];
    snprintf(error_str, sizeof(error_str), "E%02d", state_.error_code);
    error_text_sensor_->publish_state(error_str);
  } else if (error_text_sensor_ != nullptr) {
    error_text_sensor_->publish_state("OK");
  }
}

// Control methods
void BestwaySpa::set_heater(bool state) {
  if (state_.heater != state) {
    PendingCommand cmd = {BTN_HEATER, millis(), false};
    command_queue_.push_back(cmd);
    ESP_LOGD(TAG, "Queued heater %s", state ? "ON" : "OFF");
  }
}

void BestwaySpa::set_filter(bool state) {
  if (state_.filter != state) {
    PendingCommand cmd = {BTN_FILTER, millis(), false};
    command_queue_.push_back(cmd);
    ESP_LOGD(TAG, "Queued filter %s", state ? "ON" : "OFF");
  }
}

void BestwaySpa::set_bubbles(bool state) {
  if (state_.bubbles != state) {
    PendingCommand cmd = {BTN_BUBBLES, millis(), false};
    command_queue_.push_back(cmd);
    ESP_LOGD(TAG, "Queued bubbles %s", state ? "ON" : "OFF");
  }
}

void BestwaySpa::set_lock(bool state) {
  if (state_.locked != state) {
    PendingCommand cmd = {BTN_LOCK, millis(), false};
    command_queue_.push_back(cmd);
    ESP_LOGD(TAG, "Queued lock %s", state ? "ON" : "OFF");
  }
}

void BestwaySpa::set_unit(bool celsius) {
  if (state_.unit_celsius != celsius) {
    PendingCommand cmd = {BTN_UNIT, millis(), false};
    command_queue_.push_back(cmd);
    ESP_LOGD(TAG, "Queued unit toggle to %s", celsius ? "C" : "F");
  }
}

void BestwaySpa::adjust_target_temp(int8_t delta) {
  ButtonCode button = delta > 0 ? BTN_TEMP_UP : BTN_TEMP_DOWN;
  int8_t steps = abs(delta);
  
  for (int8_t i = 0; i < steps; i++) {
    PendingCommand cmd = {button, millis() + (i * 200), false};
    command_queue_.push_back(cmd);
  }
  
  ESP_LOGD(TAG, "Queued %d temperature %s steps", steps, delta > 0 ? "up" : "down");
}

}  // namespace bestway_spa
}  // namespace esphome
