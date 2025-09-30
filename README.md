# ESPHome Configuration for Bestway Lay-Z-SPA WiFi Control

This is an ESPHome implementation of the WiFi remote control for Bestway Lay-Z-SPA hot tubs, based on the reverse-engineered protocol from the [WiFi-remote-for-Bestway-Lay-Z-SPA](https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA) project.

## Features

- ✅ Full control of heater, filter, bubbles, and jets (model dependent)
- ✅ Temperature monitoring and control
- ✅ Home Assistant integration with climate control
- ✅ MQTT support for external integration
- ✅ Web interface for standalone control
- ✅ Safety features (temperature limits, auto-shutoff)
- ✅ Scheduled filter cycles
- ✅ Error monitoring and reporting
- ✅ Support for both 4-wire and 6-wire pump models

## Hardware Requirements

### Components Needed

1. **ESP8266 Board** (recommended: Wemos D1 Mini)
2. **Logic Level Converter** (3.3V ↔ 5V)
   - For 6-wire models: Bidirectional level shifter (e.g., TXS0108E)
   - For 4-wire models: Can often work without level shifter
3. **Connecting Cables**
   - 6-pin or 4-pin ribbon cable (depending on your model)
   - Dupont wires for connections
4. **Optional**: Project box/enclosure for mounting

### Supported Pump Models

#### 6-Wire Models
- NO54173 (Paris, 2021)
- NO54144 (Helsinki)
- NO54148 (Palm Springs)
- NO54150 (Monaco)
- And other 6-wire variants

#### 4-Wire Models
- NO54138
- NO54154
- NO54149E
- And other 4-wire variants

## Wiring Diagram

### 6-Wire Connection

```
Pump Display Cable          ESP8266 (via Level Shifter)
------------------          ---------------------------
Pin 1 (5V)        --------> VIN/5V
Pin 2 (GND)       --------> GND
Pin 3 (CIO Data)  <-------> D1 (GPIO5)
Pin 4 (CIO Clock) <-------> D2 (GPIO4)
Pin 5 (CIO CS)    <-------> D3 (GPIO0)
Pin 6 (DSP Data)  <-------> D5 (GPIO14)
Pin 7 (DSP Clock) <-------> D6 (GPIO12)
Pin 8 (DSP CS)    <-------> D7 (GPIO13)
```

### 4-Wire Connection

```
Pump Display Cable          ESP8266
------------------          -------
Pin 1 (5V)        --------> VIN/5V
Pin 2 (GND)       --------> GND
Pin 3 (DSP TX)    --------> D5 (GPIO14)
Pin 4 (CIO TX)    --------> D7 (GPIO13)
```

**Note**: Wire colors may vary by manufacturer. Always verify pinout with a multimeter.

## Installation

### 1. Prepare ESPHome

If you haven't already, install ESPHome:
```bash
pip install esphome
```

Or use the ESPHome Dashboard add-on in Home Assistant.

### 2. Configure the Files

1. Copy the configuration files to your ESPHome config directory:
   - `bestway_layzspa.yaml` (basic version) OR
   - `bestway_layzspa_advanced.yaml` (advanced version with more features)
   - `spa_protocol.h` (required for advanced version)
   - `secrets.yaml.template` → rename to `secrets.yaml`

2. Edit `secrets.yaml` with your credentials:
   ```yaml
   wifi_ssid: "your_wifi_name"
   wifi_password: "your_wifi_password"
   api_encryption_key: "generate_random_key"
   ota_password: "your_ota_password"
   ```

3. Modify the configuration file:
   - Update `model_type` to match your pump model
   - Adjust pin assignments if using different GPIO pins
   - Configure features based on your model

### 3. Flash the ESP8266

Initial flash via USB:
```bash
esphome run bestway_layzspa.yaml
```

After initial setup, you can update Over-The-Air (OTA):
```bash
esphome run bestway_layzspa.yaml --device layzspa.local
```

### 4. Hardware Installation

1. **Power off the spa pump** completely (unplug from mains)
2. Remove the display unit (usually 6 screws)
3. Disconnect the ribbon cable between pump and display
4. Connect the ESP8266 module inline:
   - Pump → ESP8266 → Display
5. Mount the ESP8266 securely inside the pump housing
6. Reassemble and power on

## Configuration Options

### Model Selection

In the substitutions section, set your model type:
```yaml
substitutions:
  model_type: "6wire_2021"  # Options: 6wire_2021, 6wire_pre2021, 4wire_2021, 4wire_pre2021
```

### Pin Configuration

Adjust pins based on your wiring:
```yaml
substitutions:
  cio_data_pin: GPIO5   # D1
  cio_clk_pin: GPIO4    # D2
  cio_cs_pin: GPIO0     # D3
  dsp_data_pin: GPIO14  # D5
  dsp_clk_pin: GPIO12   # D6
  dsp_cs_pin: GPIO13    # D7
```

### Features

Enable/disable features based on your model:
```yaml
# For models without jets, comment out jets-related entities
switch:
  # - platform: template
  #   name: "${friendly_name} Jets"
  #   id: jets_control
  #   ...
```

## Home Assistant Integration

Once flashed and online, the device will automatically appear in Home Assistant if you have the ESPHome integration enabled. The native ESPHome API provides the best performance and reliability - no MQTT needed!

### Entities Created

- **Climate**: Full thermostat control
- **Sensors**: Temperature, power consumption, WiFi signal
- **Switches**: Heater, filter, bubbles, jets, lock
- **Number**: Temperature setting, filter duration
- **Select**: Operating modes
- **Button**: Quick heat, maintenance, emergency stop

### Example Automation

```yaml
automation:
  - alias: "Heat spa before evening"
    trigger:
      - platform: time
        at: "17:00:00"
    condition:
      - condition: state
        entity_id: binary_sensor.someone_home
        state: 'on'
    action:
      - service: climate.set_temperature
        target:
          entity_id: climate.layzspa
        data:
          temperature: 38
      - service: climate.set_hvac_mode
        target:
          entity_id: climate.layzspa
        data:
          hvac_mode: heat
```

## Troubleshooting

### Communication Errors

1. **Check wiring** - Ensure all connections are secure
2. **Level shifter** - Some models require proper 3.3V↔5V conversion
3. **Wire length** - Keep wires under 30cm for reliability
4. **Power supply** - Ensure ESP8266 has stable 5V power

### Display Issues

- If display shows "---", check DSP connections
- If display flashes, you may need pull-up resistors (510Ω) on data lines
- For button lockouts, check the display lock status

### WiFi Issues

- Ensure WiFi signal is strong enough
- Try channel 8 if having connectivity issues
- Use static IP if DHCP is unreliable

### Model-Specific Issues

**4-Wire Models**: 
- May not support all features
- Communication might be one-way only
- Filter scheduling may need adjustment

**6-Wire Models**:
- Ensure CIO connections are correct
- Check that model type matches your pump version

## Safety Notes

⚠️ **WARNING**: 
- Always disconnect mains power before installation
- This modification voids warranty
- Improper installation can damage the pump controller
- Water and electricity are dangerous - take proper precautions

## Advanced Features

### Custom Display Messages

Send custom text to the 7-segment display:
```yaml
service: esphome.layzspa_display_text
data:
  text: "HI"
  duration: 5
```

### Raw Command Interface

Send raw protocol commands:
```yaml
service: esphome.layzspa_send_raw_command
data:
  command: 16  # Hexadecimal command code
```

### Energy Monitoring

The configuration includes power estimation based on active components:
- Heater: ~2800W
- Filter: ~60W
- Bubbles: ~800W
- Jets: ~600W (if available)

## Updates and Maintenance

### OTA Updates

After initial USB flash, update wirelessly:
1. Build new firmware: `esphome compile bestway_layzspa.yaml`
2. Upload OTA: `esphome upload bestway_layzspa.yaml --device layzspa.local`

### Backup Configuration

Always keep backups of:
- Your working configuration file
- secrets.yaml (store securely)
- Any custom modifications

## Credits

This ESPHome implementation is based on the excellent reverse-engineering work from:
- [visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA](https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA)

## License

This configuration is provided as-is for educational purposes. Use at your own risk.

## Support

For issues specific to:
- ESPHome configuration: Check ESPHome documentation
- Protocol/hardware: Refer to the original project
- Home Assistant integration: Check HA forums

## Version History

- v1.0 - Initial ESPHome port with basic functionality
- v2.0 - Advanced version with full protocol implementation

## Contributing

Improvements and model-specific configurations are welcome! Please test thoroughly before submitting.
