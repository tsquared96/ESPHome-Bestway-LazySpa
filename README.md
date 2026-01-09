# Bestway Lay-Z-Spa ESPHome Component

**Native ESPHome component for Bestway/Lay-Z-Spa hot tubs using VisualApproach hardware.**

This project provides an ESPHome-native implementation that integrates directly with Home Assistant, eliminating the need for MQTT while maintaining full compatibility with the VisualApproach hardware design.

## Credits & Acknowledgments

This project is built upon the excellent work of **[VisualApproach](https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA)**:

- **Hardware Design**: The PCB design, level shifter circuits, and connector specifications are from VisualApproach
- **Protocol Implementation**: The CIO/DSP communication code is adapted directly from the VisualApproach firmware
- **Protocol Documentation**: All protocol reverse engineering and documentation comes from the VA project

The VisualApproach project is licensed under **GPL v3**. This ESPHome component adapts their protocol handlers for the ESPHome framework while maintaining the same proven communication logic.

### What's the Same as VisualApproach

- **Hardware**: Uses the exact same VisualApproach PCB design (no hardware changes needed)
- **Protocol Handling**: CIO/DSP packet parsing, button codes, state machines from VA code
- **MITM Architecture**: Same dual-bus man-in-the-middle design intercepting CIO↔DSP communication
- **Supported Models**: All models supported by VA are supported here
- **Pin Assignments**: Same GPIO pin configurations

### What's Different

| Feature | VisualApproach | This ESPHome |
|---------|----------------|--------------|
| **Integration** | MQTT broker required | Native Home Assistant API |
| **Configuration** | Web UI | YAML files |
| **Updates** | Custom OTA | ESPHome OTA |
| **Framework** | Custom Arduino | ESPHome framework |
| **Web Interface** | Built-in full UI | Simple ESPHome web server |
| **Logging** | Serial/debug | WiFi logs via ESPHome |
| **Climate Mode/Action** | Has "idle" mode bug (#919) | Correctly separates mode vs action |

## Architecture

### MITM Dual-Bus Design

The VisualApproach hardware implements a man-in-the-middle (MITM) architecture:

```
┌─────────────┐     CIO Bus      ┌─────────────┐     DSP Bus      ┌─────────────┐
│    CIO      │ ←──────────────→ │    ESP      │ ←──────────────→ │    DSP      │
│ (Pump/Heat  │   (CLK,DATA,CS)  │  (ESPHome)  │   (CLK,DATA,CS)  │  (Display)  │
│ Controller) │                  │             │                  │   Panel     │
└─────────────┘                  └─────────────┘                  └─────────────┘
```

**CIO (Control I/O)**: The pump/heater controller in the spa. Sends status packets.
**DSP (Display)**: The physical display panel with buttons. Receives display data, sends button presses.
**ESP**: Sits in the middle, intercepts both directions, can inject button presses.

### Data Flow

1. **CIO → ESP**: ESP receives status (temp, heater, pump states)
2. **ESP → DSP**: ESP forwards display data to physical panel
3. **DSP → ESP**: ESP reads button presses from physical panel
4. **ESP → CIO**: ESP can inject virtual button presses for Home Assistant control

## Supported Protocols & Models

### 6-Wire TYPE1 (Pre-2021 "Egg-Style" Pumps)

SPI-like protocol with 11-byte packets. Used by older spas with the egg-shaped pump housing.

| Model | Jets | Air | Notes |
|-------|------|-----|-------|
| PRE2021 | No | Yes | Standard pre-2021 model |
| P05504 | No | Yes | Variant with different button codes |

**Features specific to TYPE1:**
- 72-hour hibernate timer (auto-wake feature available)
- Interrupt-driven packet reception
- 16-bit button codes

### 6-Wire TYPE2 (54149E)

Different bit ordering and 5-byte packets.

| Model | Jets | Air | Notes |
|-------|------|-----|-------|
| 54149E | Yes | Yes | Newer 6-wire protocol |

### 4-Wire UART (2021+ Models)

Standard UART at 9600 baud with 7-byte packets. Uses SoftwareSerial for DSP communication.

| Model | Jets | Air | Notes |
|-------|------|-----|-------|
| 54123 | No | Yes | Bubbles only |
| 54138 | Yes | Yes | Full featured |
| 54144 | Yes | No | Jets only |
| 54154 | No | Yes | Bubbles only |
| 54173 | Yes | Yes | Full featured |

## Features

### Climate Entity
- Full thermostat control with current and target temperature
- Mode: Heat, Fan Only (filter), Off
- Action: Heating, Idle, Fan, Off (correctly implemented - no VA bug #919)

### Control Switches
- Power on/off
- Heater on/off
- Filter pump on/off
- Air bubbles on/off
- Hydrojets on/off (model-dependent)
- Child lock on/off
- Temperature unit toggle (Celsius/Fahrenheit)

### Sensors
- Current water temperature
- Target temperature setpoint
- All state binary sensors (power, heating, filter, bubbles, jets, locked, error)
- Error code text sensor
- Display text sensor (raw 3-character display)

### Anti-Hibernate Feature (TYPE1 Only)

Older "egg-style" pumps (6-wire TYPE1) have a built-in 72-hour timer that puts the spa into hibernate mode, displaying "END" on the panel. This feature:

- **Detects** the "END" display state automatically
- **Auto-wakes** the spa by pressing the lock button for 3 seconds
- **Enables year-round operation** without manual intervention every 72 hours

```yaml
climate:
  - platform: bestway_spa
    prevent_hibernate: true  # Default: true (only applies to TYPE1)
```

**Note**: This 72-hour timer is reportedly only present in older egg-style pumps. Newer models (TYPE2, 4-wire) likely don't have this limitation.

## Hardware Requirements

**You need the VisualApproach PCB hardware.** This project only provides firmware.

### Required Components
- ESP8266 D1 Mini (or NodeMCU)
- VisualApproach PCB with TXS0108E level shifter
- Appropriate connector for your spa model

### Hardware Resources
- [VisualApproach GitHub](https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA)
- [Build Manual (PDF)](https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA/blob/master/bwc-manual.pdf)
- [EasyEDA/OSHWLab PCB Project](https://oshwlab.com/visualapproach/bestway-wireless-controller-2_copy)

**WARNING**: Never connect ESP directly to spa without level shifters - you will damage both!

## Pin Configuration

### 6-Wire Models (MITM Dual-Bus)

Matching VisualApproach firmware defaults:

```
CIO Bus (from pump controller - INPUT):
  GPIO5  (D1) = CIO Data
  GPIO4  (D2) = CIO Clock
  GPIO14 (D5) = CIO Chip Select

DSP Bus (to physical display - OUTPUT):
  GPIO12 (D6) = DSP Data
  GPIO2  (D4) = DSP Clock
  GPIO0  (D3) = DSP Chip Select
  GPIO13 (D7) = DSP Audio (optional buzzer)
```

**Note:** These match the VA firmware defaults. If you customized pins in VA, update YAML accordingly.

### 4-Wire Models (UART)

```
GPIO1 (TX) = Spa communication
GPIO3 (RX) = Spa communication
```

## Installation

### 1. Clone Repository

```bash
git clone https://github.com/tsquared96/ESPHome-Bestway-LazySpa.git
cd ESPHome-Bestway-LazySpa
```

### 2. Configure Secrets

```bash
cp secrets.yaml.example secrets.yaml
# Edit secrets.yaml with your WiFi credentials
```

### 3. Choose Your Model Configuration

Pre-configured YAML files are provided for each model:

| File | Model | Protocol |
|------|-------|----------|
| `bestway-spa-pre2021.yaml` | PRE2021/P05504 | 6-wire TYPE1 |
| `bestway-spa-54149e.yaml` | 54149E | 6-wire TYPE2 |
| `bestway-spa-54123.yaml` | 54123 | 4-wire UART |
| `bestway-spa-54138.yaml` | 54138 | 4-wire UART |
| `bestway-spa-54144.yaml` | 54144 | 4-wire UART |
| `bestway-spa-54154.yaml` | 54154 | 4-wire UART |
| `bestway-spa-54173.yaml` | 54173 | 4-wire UART |

### 4. Compile and Flash

```bash
# First time (USB required)
esphome run bestway-spa-pre2021.yaml

# OTA updates
esphome run bestway-spa-pre2021.yaml --device bestway-spa.local
```

### 5. Add to Home Assistant

The device auto-discovers via ESPHome integration. Enter your API encryption key when prompted.

## Configuration Reference

### Full 6-Wire TYPE1 Example

```yaml
climate:
  - platform: bestway_spa
    id: spa
    name: "Hot Tub"

    # Protocol and model
    protocol_type: 6WIRE    # or 6WIRE_T1
    model: PRE2021          # or P05504

    # Anti-hibernate (TYPE1 only)
    prevent_hibernate: true

    # CIO bus pins (input from pump controller) - VA defaults
    cio_data_pin: GPIO5
    cio_clk_pin: GPIO4
    cio_cs_pin: GPIO14

    # DSP bus pins (output to physical display) - VA defaults
    dsp_data_pin: GPIO12
    dsp_clk_pin: GPIO2
    dsp_cs_pin: GPIO0
    audio_pin: GPIO13

    # Sensors
    current_temperature:
      name: "Hot Tub Temperature"
    heating:
      name: "Hot Tub Heating"
    filter:
      name: "Hot Tub Filter"
    bubbles:
      name: "Hot Tub Bubbles"
    locked:
      name: "Hot Tub Locked"
    power:
      name: "Hot Tub Power"
    error:
      name: "Hot Tub Error"
    error_text:
      name: "Hot Tub Error Code"
    display_text:
      name: "Hot Tub Display"

switch:
  - platform: bestway_spa
    spa_id: spa
    heater:
      name: "Hot Tub Heater"
    filter:
      name: "Hot Tub Filter"
    bubbles:
      name: "Hot Tub Bubbles"
    lock:
      name: "Hot Tub Lock"
    power:
      name: "Hot Tub Power"
    unit:
      name: "Hot Tub Unit (C/F)"
```

### Protocol Type Options

| Value | Description |
|-------|-------------|
| `4WIRE` | Standard UART (2021+ models) |
| `6WIRE` | 6-wire TYPE1 (alias for 6WIRE_T1) |
| `6WIRE_T1` | 6-wire TYPE1 (PRE2021, P05504) |
| `6WIRE_T2` | 6-wire TYPE2 (54149E) |

## Home Assistant Examples

### Thermostat Card

```yaml
type: thermostat
entity: climate.hot_tub
```

### Automation: Heat Before Evening

```yaml
automation:
  - alias: "Heat Spa for Evening"
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

### Automation: Filter Cycles

```yaml
automation:
  - alias: "Spa Filter Cycle"
    trigger:
      - platform: time
        at: "08:00:00"
      - platform: time
        at: "20:00:00"
    action:
      - service: switch.turn_on
        entity_id: switch.hot_tub_filter
      - delay: "02:00:00"
      - service: switch.turn_off
        entity_id: switch.hot_tub_filter
```

## Troubleshooting

### No Communication

1. Check level shifter is powered and TXS0108E (not TXB0108)
2. Verify pin assignments match your wiring
3. Check spa cable connector is fully seated
4. View logs: `esphome logs bestway-spa.yaml`

### Commands Don't Work (6-Wire)

For 6-wire models, both CIO and DSP buses must be connected:
- CIO bus: Read-only monitoring works with just CIO pins
- DSP bus: Required for sending commands (button presses)

If DSP pins aren't configured, you'll see a warning:
```
dsp_data_pin not configured - button control will NOT work!
```

### Display Shows "END" (TYPE1)

This is the 72-hour hibernate state. With `prevent_hibernate: true` (default), the spa will auto-wake. Check logs for:
```
Hibernate state detected (display shows END)
Attempting to wake spa from hibernate (pressing LOCK for 3s)
Spa woke from hibernate state
```

### Climate Shows Wrong Mode

The VA firmware bug (#919) sent "idle" as a mode instead of an action. This ESPHome implementation correctly:
- Uses `mode: HEAT` when heater is enabled (stays HEAT even when not actively heating)
- Uses `action: HEATING` when actively heating
- Uses `action: IDLE` when at temperature but heater enabled

## Component Structure

```
components/bestway_spa/
├── __init__.py          # ESPHome component registration
├── climate.py           # Climate platform schema
├── switch.py            # Switch platform schema
├── bestway_spa.h        # Main component header
├── bestway_spa.cpp      # Main component implementation
├── enums.h              # Shared enums and structures
├── ports.h              # GPIO definitions
├── CIO_TYPE1.h/cpp      # TYPE1 CIO handler (from VA)
├── DSP_TYPE1.h/cpp      # TYPE1 DSP handler (from VA)
├── CIO_TYPE2.h/cpp      # TYPE2 CIO handler (from VA)
├── DSP_TYPE2.h/cpp      # TYPE2 DSP handler (from VA)
├── CIO_4W.h/cpp         # 4-wire CIO handler (from VA)
└── DSP_4W.h/cpp         # 4-wire DSP handler (from VA)
```

## License

This project incorporates code from VisualApproach's WiFi-remote-for-Bestway-Lay-Z-SPA, which is licensed under GPL v3.

- Original VisualApproach code: GPL v3
- ESPHome integration layer: MIT License

## Links

- [VisualApproach Original Project](https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA)
- [VisualApproach Build Manual](https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA/blob/master/bwc-manual.pdf)
- [ESPHome Documentation](https://esphome.io/)

## Changelog

### v2.1.0 (2026-01-07)
- **Anti-hibernate feature** for TYPE1 models (72-hour timer auto-wake)
- Per-model YAML configuration files
- Updated documentation with VA credits and architecture details

### v2.0.0 (2026-01-01)
- Complete TYPE1, TYPE2, and 4-wire protocol support using VA code
- MITM dual-bus architecture (CIO + DSP)
- All Bestway models supported
- Unit switch (Celsius/Fahrenheit)
- Button enable/disable controls
- Fixed climate mode/action (VA bug #919)

### v1.0.0 (2025-12-01)
- Initial release with basic 4-wire support
