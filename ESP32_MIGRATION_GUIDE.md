# ESP32 Migration Guide

## Can I Use ESP32 Instead of ESP8266?

**YES!** The ESP32 works great for this project and has several advantages.

## ESP32 Advantages

### ✅ Benefits of Using ESP32

1. **More GPIO Pins**
   - ESP32: 34 GPIO pins available
   - ESP8266: Only ~11 usable GPIO pins
   - Better for complex projects

2. **More Memory**
   - ESP32: 520 KB SRAM
   - ESP8266: 80 KB SRAM
   - Less likely to run out of memory

3. **Faster Processor**
   - ESP32: Dual-core 240 MHz
   - ESP8266: Single-core 80-160 MHz
   - Better for real-time communication

4. **Better WiFi**
   - ESP32: Faster, more stable WiFi
   - Better range in some cases

5. **Bluetooth Support**
   - ESP32 has Bluetooth/BLE
   - Could add Bluetooth control in future

6. **No Boot Pin Issues**
   - ESP32 has more flexibility with pin selection
   - Fewer pins that cause boot problems

### ⚠️ Potential Drawbacks

1. **Higher Power Consumption**
   - ESP32 uses more power (~80-260mA vs 60-170mA)
   - Not an issue if powered from spa pump

2. **Slightly More Expensive**
   - ESP32: ~$4-8
   - ESP8266: ~$2-5
   - Still very affordable

3. **Different Pin Numbers**
   - Must update configuration (see below)

## What Needs to Change

### 1. Hardware Configuration

**ESP8266 (D1 Mini) Pins:**
```yaml
esp8266:
  board: d1_mini

cio_data_pin: GPIO5   # D1
cio_clk_pin: GPIO4    # D2
cio_cs_pin: GPIO0     # D3
dsp_data_pin: GPIO14  # D5
dsp_clk_pin: GPIO12   # D6
dsp_cs_pin: GPIO13    # D7
```

**ESP32 (DevKit v1) Pins:**
```yaml
esp32:
  board: esp32dev
  framework:
    type: arduino

cio_data_pin: GPIO21  # No "D" labels on ESP32
cio_clk_pin: GPIO22
cio_cs_pin: GPIO23
dsp_data_pin: GPIO19
dsp_clk_pin: GPIO18
dsp_cs_pin: GPIO5
```

### 2. Wiring Changes

The physical connections remain the same, but you connect to different GPIO pins:

#### ESP8266 Wiring:
```
Spa → Level Shifter → ESP8266 D1 Mini
Pin 3 → HV1↔LV1 → D1 (GPIO5)
Pin 4 → HV2↔LV2 → D2 (GPIO4)
Pin 5 → HV3↔LV3 → D3 (GPIO0)
Pin 6 → HV4↔LV4 → D5 (GPIO14)
Pin 7 → HV5↔LV5 → D6 (GPIO12)
Pin 8 → HV6↔LV6 → D7 (GPIO13)
```

#### ESP32 Wiring:
```
Spa → Level Shifter → ESP32 DevKit
Pin 3 → HV1↔LV1 → GPIO21
Pin 4 → HV2↔LV2 → GPIO22
Pin 5 → HV3↔LV3 → GPIO23
Pin 6 → HV4↔LV4 → GPIO19
Pin 7 → HV5↔LV5 → GPIO18
Pin 8 → HV6↔LV6 → GPIO5
```

## ESP32 Pin Selection Guide

### Safe Pins to Use (Input/Output)

These pins are safe for general use:
- **GPIO21, GPIO22** - I2C pins (SDA, SCL) - Good for data
- **GPIO23** - Safe output pin
- **GPIO19, GPIO18** - SPI pins (MISO, SCK) - Good for data
- **GPIO5** - SPI CS pin
- **GPIO16, GPIO17** - UART2 pins
- **GPIO25, GPIO26, GPIO27** - DAC pins
- **GPIO32, GPIO33** - ADC pins

### Pins to AVOID on ESP32

❌ **DO NOT USE:**
- **GPIO0** - Boot mode pin (pulled HIGH, boot fails if LOW)
- **GPIO2** - On-board LED, strapping pin
- **GPIO12** - Strapping pin (boot fails if HIGH during startup)
- **GPIO15** - Strapping pin
- **GPIO6 to GPIO11** - Connected to internal flash (will brick your ESP32!)

❌ **Input Only (can't use for output):**
- **GPIO34, GPIO35, GPIO36, GPIO39** - Input only, no internal pull-up

## File Comparison

### For ESP8266:
- Use: `spa_diagnostic.yaml` (if it exists)
- Use: `bestway_lazyspa_advanced.yaml`

### For ESP32:
- Use: `spa_diagnostic_esp32.yaml` ✅ (Created for you!)
- Create: `bestway_lazyspa_advanced_esp32.yaml` (see below)

## Quick Migration Steps

### Option 1: Use the ESP32 Diagnostic File (Easiest)

I've created `spa_diagnostic_esp32.yaml` for you with correct ESP32 pins!

```bash
# Just flash it!
esphome run spa_diagnostic_esp32.yaml
```

### Option 2: Convert Any ESP8266 Config to ESP32

To convert any ESP8266 config to ESP32:

1. **Change the platform section:**
   ```yaml
   # OLD (ESP8266)
   esphome:
     platform: ESP8266
     board: d1_mini

   # NEW (ESP32)
   esphome:
     # Remove platform and board from here

   esp32:
     board: esp32dev
     framework:
       type: arduino
   ```

2. **Update GPIO pin numbers:**
   ```yaml
   # OLD (ESP8266)
   dsp_data_pin: GPIO14  # D5

   # NEW (ESP32)
   dsp_data_pin: GPIO19  # No "D" labels
   ```

3. **Update any D1 Mini specific code:**
   - Remove references to D1, D2, D3, etc. (ESP32 doesn't use these labels)
   - Use GPIO numbers directly

## ESP32 Board Types

Different ESP32 boards work with this project:

### ESP32 DevKit v1 (Most Common)
```yaml
esp32:
  board: esp32dev
```
- 30 GPIO pins exposed
- Micro USB
- ~$5-7
- **Recommended for beginners**

### ESP32-WROOM-32
```yaml
esp32:
  board: esp32dev
```
- Same as DevKit v1
- Just a different brand name

### ESP32-S2/S3/C3 (Newer Variants)
```yaml
esp32:
  board: esp32-s2-saola-1  # or esp32-s3-devkitc-1, etc.
```
- Different pin layouts
- Check specific pinout before using

### NodeMCU-32S
```yaml
esp32:
  board: nodemcu-32s
```
- Similar to DevKit but different layout
- Works fine for this project

## Testing Your ESP32 Setup

### Step 1: Flash Diagnostic Config

```bash
esphome run spa_diagnostic_esp32.yaml
```

### Step 2: Check Serial Output

```bash
esphome logs spa_diagnostic_esp32.yaml
```

Look for:
```
[I][wifi:xxx] WiFi Connected!
[I][app:xxx] ESP32 ready!
[D][pins:xxx] Pin states - CS:0 CLK:0 DATA:0
```

### Step 3: Verify Communication

Turn on spa pump and watch for:
```
[I][diagnostic:xxx] DSP CS activated - pump is sending data
[I][diagnostic:xxx] Communication OK - received XX packets
```

If packet count increases → **ESP32 setup is working!**

## Power Considerations

### ESP32 Power Requirements

- **Input Voltage:** 3.3V (logic) + 5V (USB/VIN)
- **Current Draw:** 80-260mA (higher than ESP8266)
- **Peak Current:** Can spike to 500mA during WiFi transmit

### Powering from Spa Pump

The spa pump provides 5V which is fine for ESP32:

```
Spa 5V → ESP32 VIN pin (built-in regulator converts to 3.3V)
Spa GND → ESP32 GND
```

**Make sure:**
- Level shifter is powered from ESP32's 3.3V pin (LV side)
- Level shifter HV side powered from spa's 5V

## Common Issues and Solutions

### Issue 1: ESP32 Won't Boot

**Symptoms:** Bootloop, constant restarts

**Solutions:**
1. Check if GPIO12 is HIGH during boot (causes boot failure)
   - Don't use GPIO12 for signals that might be HIGH at startup
2. Ensure GPIO0 is not pulled LOW
3. Check power supply can provide enough current (500mA+)

### Issue 2: Pin Conflicts

**Symptoms:** Flash fails, boot problems, weird behavior

**Solutions:**
- Avoid GPIO6-11 (flash pins)
- Don't use GPIO34-39 for outputs
- Use the recommended safe pins listed above

### Issue 3: WiFi Weaker Than ESP8266

**Symptoms:** Poor WiFi signal

**Solutions:**
1. ESP32 antenna orientation matters more
2. Keep antenna away from metal
3. External antenna ESP32 boards available

## Pin Reference Table

| Function | ESP8266 Pin | ESP32 Pin | Notes |
|----------|-------------|-----------|-------|
| CIO Data | GPIO5 (D1) | GPIO21 | I2C SDA |
| CIO Clock | GPIO4 (D2) | GPIO22 | I2C SCL |
| CIO CS | GPIO0 (D3) | GPIO23 | Safe pin |
| DSP Data | GPIO14 (D5) | GPIO19 | SPI MISO |
| DSP Clock | GPIO12 (D6) | GPIO18 | SPI SCK |
| DSP CS | GPIO13 (D7) | GPIO5 | SPI CS |
| Power | 5V/VIN | 5V/VIN | Same |
| Ground | GND | GND | Same |
| 3.3V Out | 3.3V | 3.3V | For level shifter |

## Recommendation

**For this project, ESP32 is actually BETTER than ESP8266:**

✅ More reliable communication (faster processor)
✅ More memory (less likely to crash)
✅ More pins (easier to add features later)
✅ Better WiFi stability
✅ Same price range

**Use ESP32 if you have one!**

## Quick Start for ESP32

1. Wire your ESP32 using the pin mapping above
2. Flash the ESP32 diagnostic config:
   ```bash
   esphome run spa_diagnostic_esp32.yaml
   ```
3. Watch logs to verify communication works
4. If successful, migrate to advanced config with ESP32 pins

## Need Help?

If you're switching to ESP32:
1. Start with `spa_diagnostic_esp32.yaml` (ready to use!)
2. Verify packet count increases
3. Then convert advanced config by changing pins
4. Test incrementally

**Bottom line: Yes, ESP32 works great and is recommended!**
