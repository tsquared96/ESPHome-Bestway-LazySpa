# NOTE THIS IS A WORK IN PROGRESS - I HAVE NOT YET SORTED OUT ALL THE DETAILS BUT I AM GETTING CLOSER!

# Bestway Lay-Z-Spa ESPHome Firmware

**Native ESPHome firmware for Bestway spas using VisualApproach hardware.**

This completely replaces the VisualApproach firmware with pure ESPHome, giving you native Home Assistant integration with no MQTT broker needed.

## Overview

```
[Bestway Spa] ←serial/SPI→ [ESP8266 + ESPHome] ←API→ [Home Assistant]
```

**You keep the existing VisualApproach PCB hardware** (level shifters, connectors, etc.) but replace the firmware with this ESPHome implementation.

## Why Replace VisualApproach Firmware?

- **Simpler** - No MQTT broker needed
- **Faster** - Direct Home Assistant API
- **Cleaner** - Native ESPHome component
- **Better integration** - Automatic discovery, services, automations
- **OTA updates** - Update from Home Assistant UI
- **More reliable** - ESPHome's proven stability

## Features

### Climate Entity
- Full thermostat control with temperature display (current & target)
- Heat/Fan/Off modes with heating action status

### Switches
- Power on/off
- Heater on/off
- Filter pump on/off
- Air bubbles on/off
- Hydrojets on/off (model-dependent)
- Child lock

### Sensors
- Current temperature (with smoothing)
- Target temperature
- Power status (binary)
- Heating status (binary)
- Filter running (binary)
- Bubbles running (binary)
- Jets running (binary)
- Lock status (binary)
- Error detection (binary)
- Error code (text)
- Display text

## Supported Protocols & Models

### 4-Wire UART (2021+ models)
Standard UART protocol at 9600 baud. Most common on newer spas.

| Model | Jets | Air | Notes |
|-------|------|-----|-------|
| 54123 | No | No | Basic model |
| 54138 | Yes | Yes | Full featured |
| 54144 | Yes | No | Jets only |
| 54154 | No | No | Basic model (default) |
| 54173 | Yes | Yes | Full featured |

### 6-Wire SPI-like (Pre-2021 models)
Uses CLK, DATA, and CS pins for communication. Two protocol variants:

**TYPE1 Models:**
| Model | Jets | Air | Notes |
|-------|------|-----|-------|
| PRE2021 | No | Yes | Standard pre-2021 |
| P05504 | No | Yes | Variant with different button codes |

**TYPE2 Models:**
| Model | Jets | Air | Notes |
|-------|------|-----|-------|
| 54149E | No | Yes | Different bit ordering |

## Hardware Requirements

### Required Components
You need the **VisualApproach PCB** which includes:
- ESP8266 (NodeMCU or compatible)
- TXS0108E bidirectional level shifter (8-channel)
- Proper connectors for spa cable
- Optional: 510-560Ω resistors on signal lines

### Where to Get Hardware
- [EasyEDA/OSHWLab Project](https://oshwlab.com/visualapproach/bestway-wireless-controller-2_copy)
- Order PCB_V2B design
- See [VisualApproach build instructions (PDF)](https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA/blob/master/bwc-manual.pdf)
- [VisualApproach GitHub Repository](https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA)

## Pin Configuration

### 4-Wire Models (UART)
```
GPIO1 (TX)  → Level Shifter → Spa TX
GPIO3 (RX)  → Level Shifter → Spa RX
GPIO2       → Built-in LED (status)
```

### 6-Wire Models (SPI-like)
```
GPIO14 (D5) → Level Shifter → CLK (Clock)
GPIO12 (D6) → Level Shifter → DATA (Bidirectional)
GPIO13 (D7) → Level Shifter → CS (Chip Select)
GPIO15 (D8) → Level Shifter → AUDIO (Optional buzzer)
GPIO2       → Built-in LED (status)
```

**IMPORTANT:** Never connect ESP directly to spa - you will damage it! Always use level shifters.

## Installation

### 1. Clone This Repository

```bash
git clone https://github.com/tsquared96/ESPHome-Bestway-LazySpa.git
cd ESPHome-Bestway-LazySpa
```

### 2. Configure Secrets

```bash
cp secrets.yaml.example secrets.yaml
nano secrets.yaml
```

Fill in your WiFi credentials and encryption keys.

### 3. Edit Configuration

Edit `bestway-spa.yaml` to match your spa model:

**For 4-wire (2021+) models:**
```yaml
climate:
  - platform: bestway_spa
    protocol_type: 4WIRE
    model: "54154"  # Change to match your model
```

**For 6-wire TYPE1 (pre-2021) models:**
```yaml
climate:
  - platform: bestway_spa
    protocol_type: 6WIRE
    model: PRE2021
    clk_pin: GPIO14
    data_pin: GPIO12
    cs_pin: GPIO13
    audio_pin: GPIO15
```

**For 6-wire TYPE2 (54149E) models:**
```yaml
climate:
  - platform: bestway_spa
    protocol_type: 6WIRE_T2
    model: 54149E
    clk_pin: GPIO14
    data_pin: GPIO12
    cs_pin: GPIO13
```

### 4. Compile and Flash

**First time (USB cable required):**
```bash
esphome run bestway-spa.yaml
```

**Subsequent updates (OTA):**
```bash
esphome run bestway-spa.yaml --device bestway-spa.local
```

### 5. Add to Home Assistant

Device auto-discovers via ESPHome API:
1. Go to Settings → Devices & Services
2. Click "Configure" on discovered ESPHome device
3. Enter encryption key (from your secrets.yaml)

## Configuration Reference

### Protocol Types
| Value | Description |
|-------|-------------|
| `4WIRE` | Standard UART (2021+ models) |
| `6WIRE` | SPI-like TYPE1 (pre-2021) |
| `6WIRE_T1` | Same as 6WIRE |
| `6WIRE_T2` | SPI-like TYPE2 (54149E) |

### Model Options
| Value | Protocol | Jets | Air |
|-------|----------|------|-----|
| `PRE2021` | 6WIRE_T1 | No | Yes |
| `P05504` | 6WIRE_T1 | No | Yes |
| `54149E` | 6WIRE_T2 | No | Yes |
| `54123` | 4WIRE | No | No |
| `54138` | 4WIRE | Yes | Yes |
| `54144` | 4WIRE | Yes | No |
| `54154` | 4WIRE | No | No |
| `54173` | 4WIRE | Yes | Yes |

### Available Sensors

**Temperature Sensors:**
- `current_temperature` - Water temperature
- `target_temperature` - Set point

**Binary Sensors:**
- `power` - Power state
- `heating` - Heater active
- `filter` - Filter pump running
- `bubbles` - Air bubbles active
- `jets` - Hydrojets active (if supported)
- `locked` - Child lock engaged
- `error` - Error detected

**Text Sensors:**
- `error_text` - Error code (E01, E02, etc.)
- `display_text` - Current display content

### Available Switches

- `bestway_spa_power` - Power control
- `bestway_spa_heater` - Heater control
- `bestway_spa_filter` - Filter pump control
- `bestway_spa_bubbles` - Bubbles control
- `bestway_spa_jets` - Jets control (model-dependent)
- `bestway_spa_lock` - Child lock control

## Home Assistant Examples

### Dashboard Card

```yaml
type: thermostat
entity: climate.hot_tub
```

### Automation: Heat Before Evening

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
          temperature: 39
      - service: climate.set_hvac_mode
        target:
          entity_id: climate.hot_tub
        data:
          hvac_mode: heat
```

### Automation: Filter Cycle

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

### Automation: Bubbles Party Mode

```yaml
automation:
  - alias: "Spa Party Mode"
    trigger:
      - platform: state
        entity_id: input_boolean.spa_party_mode
        to: "on"
    action:
      - service: switch.turn_on
        target:
          entity_id:
            - switch.hot_tub_bubbles
            - switch.hot_tub_jets
```

## Troubleshooting

### Climate shows "unavailable"

**Check connections:**
- Verify UART/SPI pins are correct
- Ensure level shifter is working
- Check spa cable is connected

**Check logs:**
```bash
esphome logs bestway-spa.yaml
```

Look for:
- `Setting up Bestway Spa...`
- `Bestway Spa initialized (Protocol: ...)`
- Packet receive messages

### No temperature updates

Verify your model selection is correct. Different models have different packet formats.

### Commands don't work

**For 6-wire models:**
- Check CLK, DATA, CS pin assignments
- Verify level shifter is bidirectional (TXS0108E, not TXB0108)
- Some boards need 510-560Ω resistors on signal lines

**For 4-wire models:**
- Check TX/RX are not swapped
- Verify baud rate is 9600

### Spa shows random errors

**Level shifter issues:**
- Use TXS0108E (bidirectional) not TXB0108
- Add resistors on signal lines if needed
- Check all solder joints

## Protocol Details

### 4-Wire UART Protocol

**Settings:** 9600 baud, 8N1

**Packet Format (7 bytes):**
```
[0xFF] [CMD] [TEMP] [ERR] [RSV] [CHK] [0xFF]
```

**Command Byte Bits:**
- Bits vary by model (54123 vs 54138 etc.)
- Controls heater stages, pump, bubbles, jets

### 6-Wire SPI-like Protocol

**TYPE1 (PRE2021, P05504):**
- 11-byte payload
- MSB-first transmission
- Commands: 0x01 (mode), 0x40 (write), 0x42 (read)

**TYPE2 (54149E):**
- 5-byte payload
- LSB-first transmission
- Commands: 0x40, 0xC0, 0x88

### Button Codes (6-Wire TYPE1)

| Button | PRE2021 | P05504 |
|--------|---------|--------|
| No Button | 0x1B1B | 0x1B1B |
| Lock | 0x0200 | 0x0210 |
| Timer | 0x0100 | 0x0110 |
| Bubbles | 0x0300 | 0x0310 |
| Unit | 0x1012 | 0x1022 |
| Heat | 0x1212 | 0x1222 |
| Pump | 0x1112 | 0x1122 |
| Down | 0x1312 | 0x1322 |
| Up | 0x0809 | 0x081A |

## Development

### Component Structure

```
components/bestway_spa/
├── __init__.py         # ESPHome Python config
├── bestway_spa.h       # C++ header with enums, structs
└── bestway_spa.cpp     # C++ implementation
```

### Building

```bash
esphome compile bestway-spa.yaml
```

### Debugging

Use WiFi logging:
```bash
esphome logs bestway-spa.yaml --device bestway-spa.local
```

Set log level in YAML:
```yaml
logger:
  level: VERBOSE  # Shows all packet data
```

## Comparison: ESPHome vs VisualApproach

| Feature | VisualApproach | This ESPHome |
|---------|----------------|--------------|
| MQTT | Required | Not needed |
| Web UI | Built-in | Via HA |
| Config | Web interface | YAML |
| HA Integration | Via MQTT | Native API |
| OTA Updates | Custom | ESPHome |
| Logs | Serial only | WiFi logs |
| Automations | Limited | Full HA |
| 6-Wire Support | Full | Full |
| Models Supported | All | All |

## Credits

- **VisualApproach** - Original firmware, hardware design, and protocol documentation
- **ESPHome** - Framework and tools
- **Community** - Protocol reverse engineering and testing

## License

MIT License - See LICENSE file

## Support

- **Issues:** GitHub Issues
- **Original Hardware:** [VisualApproach GitHub](https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA)

## Changelog

### v2.0.0 (2026-01-01)
- **Complete 6-wire protocol implementation**
  - TYPE1 support (PRE2021, P05504 models)
  - TYPE2 support (54149E model)
  - Full SPI-like bit-banging
- **All models supported**
  - 4-wire: 54123, 54138, 54144, 54154, 54173
  - 6-wire: PRE2021, P05504, 54149E
- **New features**
  - Power switch
  - Jets switch (model-dependent)
  - Display text sensor
  - Model-specific button codes
  - Dual-stage heater control
- **Improved**
  - Button queue system for 6-wire
  - State tracking
  - Error handling

### v1.0.0 (2026-01-01)
- Initial release
- 4-wire UART protocol support
- Basic climate entity and controls
