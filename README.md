# Bestway Lay-Z-Spa ESPHome Firmware

**Native ESPHome firmware replacement for Bestway spas using Visualapproach hardware.**

This completely replaces the Visualapproach firmware with pure ESPHome, giving you native Home Assistant integration with no MQTT broker needed.

## Overview

```
[Bestway Spa] ←serial→ [ESP8266 + ESPHome + This Component] ←API→ [Home Assistant]
```

**You keep the existing Visualapproach PCB hardware** (level shifters, connectors, etc.) but replace the firmware with this ESPHome implementation.

## Why Replace Visualapproach Firmware?

✅ **Simpler** - No MQTT broker needed  
✅ **Faster** - Direct Home Assistant API  
✅ **Cleaner** - Native ESPHome component, not custom firmware  
✅ **Better integration** - Automatic discovery, services, automations  
✅ **OTA updates** - Update from Home Assistant UI  
✅ **More reliable** - ESPHome's proven stability  

## Features

### Climate Entity
- Full thermostat control
- Temperature display (current & target)
- Heat/Fan/Off modes
- Heating action status

### Switches
- Heater on/off
- Filter pump on/off
- Air bubbles on/off
- Child lock

### Sensors
- Current temperature (with smoothing)
- Target temperature
- Heating status (binary)
- Filter running (binary)
- Bubbles running (binary)
- Lock status (binary)
- Error detection (binary)
- Error code (text)

## Hardware Compatibility

### Supported Models

**4-Wire (2021+ models):**
- UART protocol, 9600 baud
- Most common on newer spas
- Default in this firmware

**6-Wire (Pre-2021 models):**
- SPI-like protocol
- Partially supported (work in progress)

### Required Hardware

You need the **Visualapproach PCB** which includes:
- ESP8266 (NodeMCU or compatible)
- TXS0108E level shifter
- Proper connectors for spa cable
- Optional resistors (some models)

**Get the PCB:**
- [EasyEDA Project](https://oshwlab.com/visualapproach/bestway-wireless-controller-2_copy)
- Order PCB_V2B design
- See [Visualapproach build instructions](https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA/blob/master/bwc-manual.pdf)

## Installation

### 1. Backup Current Firmware (Optional)

If you want to keep Visualapproach firmware as backup:

```bash
esptool.py --port /dev/ttyUSB0 read_flash 0x00000 0x400000 visualapproach_backup.bin
```

### 2. Clone This Repository

```bash
git clone https://github.com/yourusername/bestway-esphome.git
cd bestway-esphome
```

### 3. Configure Secrets

```bash
cp secrets.yaml.example secrets.yaml
nano secrets.yaml
```

Fill in your WiFi credentials.

### 4. Edit Configuration

Edit `bestway-spa.yaml` if needed:

```yaml
substitutions:
  friendly_name: "Hot Tub"  # Change to your preference

# For pre-2021 models with 6-wire connection:
climate:
  - platform: bestway_spa
    protocol_type: 6WIRE  # Change from 4WIRE
```

### 5. Compile and Flash

**First time (USB cable required):**

```bash
esphome run bestway-spa.yaml
```

Select your serial port when prompted.

**Subsequent updates (OTA):**

```bash
esphome run bestway-spa.yaml --device bestway-spa.local
```

Or use Home Assistant:
- ESPHome Dashboard → Click device → Install → Wirelessly

### 6. Add to Home Assistant

Device auto-discovers via ESPHome API:
1. Go to Settings → Devices & Services
2. Click "Configure" on discovered ESPHome device
3. Enter API key (shown in logs on first boot)

## Pin Configuration

### NodeMCU / ESP8266

```
GPIO1 (TX)  → Level Shifter → Spa TX
GPIO3 (RX)  → Level Shifter → Spa RX
GPIO2       → Built-in LED (status)
```

**IMPORTANT:** Logging on serial is disabled since UART is used for spa communication. Use WiFi logging instead:

```bash
esphome logs bestway-spa.yaml
```

### Wiring

The Visualapproach PCB handles all level shifting (5V spa ↔ 3.3V ESP).

**Never connect ESP directly to spa - you will damage it!**

## Usage

### Dashboard Card

```yaml
type: thermostat
entity: climate.hot_tub
```

### Automation Examples

**Heat before evening:**
```yaml
automation:
  - alias: "Heat Spa Evening"
    trigger:
      - platform: time
        at: "16:00:00"
    action:
      - service: climate.set_temperature
        target:
          entity_id: climate.hot_tub
        data:
          temperature: 102
      - service: climate.set_hvac_mode
        target:
          entity_id: climate.hot_tub
        data:
          hvac_mode: heat
```

**Energy saving overnight:**
```yaml
automation:
  - alias: "Spa Economy"
    trigger:
      - platform: time
        at: "23:00:00"
    action:
      - service: climate.set_temperature
        target:
          entity_id: climate.hot_tub
        data:
          temperature: 92
```

**Filter cycle:**
```yaml
automation:
  - alias: "Filter Cycle"
    trigger:
      - platform: time
        at: "08:00:00"
      - platform: time
        at: "20:00:00"
    action:
      - service: switch.turn_on
        target:
          entity_id: switch.hot_tub_filter_pump
      - delay: "02:00:00"
      - service: switch.turn_off
        target:
          entity_id: switch.hot_tub_filter_pump
```

## Troubleshooting

### Climate shows "unavailable"

**Check serial connection:**
- Verify UART pins are correct
- Ensure level shifter is working
- Check spa cable is connected

**Check logs:**
```bash
esphome logs bestway-spa.yaml
```

Look for:
- `Setting up Bestway Spa...`
- `Bestway Spa initialized`
- Packet receive messages

### No temperature updates

**Add temperature smoothing:**

The spa temperature sensor can be noisy. The example config includes smoothing:

```yaml
current_temperature:
  filters:
    - sliding_window_moving_average:
        window_size: 3
        send_every: 1
```

### Commands don't work

**Increase command delay:**

Some spas need more time between button presses:

```cpp
// In bestway_spa.cpp, adjust:
PendingCommand cmd = {button, millis() + (i * 300), false};  // Increase from 200
```

### Won't compile

**Check component path:**
```yaml
external_components:
  - source:
      type: local
      path: components
    components: [ bestway_spa ]
```

Path must be relative to your .yaml file.

### Spa shows random errors

**Level shifter issues:**

Some boards need resistors between level shifter and spa:
- 510-560Ω on CLK, DATA, CS lines (6-wire only)
- See Visualapproach build instructions

## Protocol Details

### 4-Wire UART Protocol

**Settings:**
- Baud rate: 9600
- Data bits: 8
- Parity: None
- Stop bits: 1

**Packet Types:**

1. **Query (CIO → DSP):**
   ```
   0x1B 0x1B  // "No button pressed"
   ```

2. **Button Code (DSP → CIO):**
   ```
   0x10 0x12  // Heater button
   0x10 0x13  // Filter button
   0x10 0x14  // Bubbles button
   0x10 0x15  // Temp up
   0x10 0x16  // Temp down
   0x10 0x17  // Lock
   0x10 0x18  // Unit toggle
   ```

3. **Display Data (CIO → DSP):**
   - Variable length packets
   - Contains LED segment data
   - Encodes temperature, LED states

**Button Codes:**
| Code | Function |
|------|----------|
| 0x1012 | Heater toggle |
| 0x1013 | Filter toggle |
| 0x1014 | Bubbles toggle |
| 0x1015 | Temperature up |
| 0x1016 | Temperature down |
| 0x1017 | Child lock toggle |
| 0x1018 | Unit (F/C) toggle |

### Man-in-the-Middle Operation

This firmware acts as a transparent proxy:
1. Forwards all packets between CIO and DSP
2. Monitors packets to extract state
3. Injects button presses when HA commands received
4. Updates HA with current state

## Development

### Component Structure

```
components/bestway_spa/
├── __init__.py         # ESPHome Python config
├── bestway_spa.h       # C++ header
└── bestway_spa.cpp     # C++ implementation
```

### Building

```bash
esphome compile bestway-spa.yaml
```

### Adding Features

To add new controls:

1. Add switch in header:
```cpp
void set_my_feature(bool state);
```

2. Implement in cpp:
```cpp
void BestwaySpa::set_my_feature(bool state) {
  PendingCommand cmd = {BTN_MY_FEATURE, millis(), false};
  command_queue_.push_back(cmd);
}
```

3. Add to Python config:
```python
BestwaySpaMyFeatureSwitch = bestway_spa_ns.class_(...)
```

4. Create switch in YAML:
```yaml
switch:
  - platform: bestway_spa_my_feature
    name: "My Feature"
    bestway_spa_id: hot_tub
```

### Testing

Use WiFi logging to monitor operation:
```bash
esphome logs bestway-spa.yaml --device bestway-spa.local
```

## Comparison vs Visualapproach

| Feature | Visualapproach | This ESPHome |
|---------|----------------|--------------|
| MQTT | Required | Not needed |
| Web UI | Built-in | Via HA |
| Config | Web interface | YAML |
| HA Integration | Via MQTT | Native API |
| OTA Updates | Custom | ESPHome |
| Logs | Serial only | WiFi logs |
| Automations | Limited | Full HA |
| Scheduling | Built-in | HA automations |

## Credits

- **Visualapproach** - Original firmware and hardware design
- **ESPHome** - Framework and tools
- **Community** - Protocol reverse engineering

## License

MIT License

## Support

- **Issues:** GitHub Issues
- **Discussions:** GitHub Discussions
- **HA Community:** Home Assistant Forums
- **ESPHome:** Discord #custom-components

## Future Plans

- [ ] Complete 6-wire protocol implementation
- [ ] Temperature calibration
- [ ] More detailed error reporting
- [ ] Power consumption estimation
- [ ] Preset modes (Eco, Comfort, etc.)
- [ ] Integration with solar systems

## Changelog

### v1.0.0 (2026-01-01)
- Initial release
- 4-wire UART protocol support
- Full climate entity
- All basic controls (heater, filter, bubbles, lock)
- Temperature sensors
- Error detection
- Native HA integration
