# Quick Wiring Verification Guide

## Your Current Configuration

Based on your `bestway_lazyspa_advanced.yaml`, you have a **6-wire 2021 model**.

## Pin Mapping Quick Reference

The diagnostic YAML is now configured with the correct pins for your model:

### Physical Connections You Should Have:

```
SPA PUMP CONNECTOR → LEVEL SHIFTER → ESP8266 D1 Mini
═══════════════════════════════════════════════════════

Pin 1 (RED wire - 5V)      →  HV Power    →  5V or VIN pin
Pin 2 (BLACK wire - GND)   →  GND         →  GND pin
Pin 3 (YELLOW - CIO Data)  →  HV1 ↔ LV1  →  D1 (GPIO5)
Pin 4 (GREEN - CIO Clock)  →  HV2 ↔ LV2  →  D2 (GPIO4)
Pin 5 (BLUE - CIO CS)      →  HV3 ↔ LV3  →  D3 (GPIO0)
Pin 6 (WHITE - DSP Data)   →  HV4 ↔ LV4  →  D5 (GPIO14)
Pin 7 (GRAY - DSP Clock)   →  HV5 ↔ LV5  →  D6 (GPIO12)
Pin 8 (PURPLE - DSP CS)    →  HV6 ↔ LV6  →  D7 (GPIO13)

Level Shifter Power:
   LV Power                →  3.3V from ESP8266
```

**NOTE:** Wire colors shown are typical but may vary. Always verify with multimeter!

## Checklist: Verify Your Wiring

Print this out and check off each connection:

### Power Connections
- [ ] Spa 5V (Pin 1) connected to Level Shifter HV
- [ ] Spa GND (Pin 2) connected to Level Shifter GND
- [ ] Level Shifter GND connected to ESP8266 GND
- [ ] Level Shifter LV connected to ESP8266 3.3V
- [ ] ESP8266 powered via USB or VIN pin

### CIO Connections (Control Input/Output)
- [ ] Spa Pin 3 (CIO Data) → Shifter HV1 ↔ LV1 → ESP D1
- [ ] Spa Pin 4 (CIO Clock) → Shifter HV2 ↔ LV2 → ESP D2
- [ ] Spa Pin 5 (CIO CS) → Shifter HV3 ↔ LV3 → ESP D3

### DSP Connections (Display)
- [ ] Spa Pin 6 (DSP Data) → Shifter HV4 ↔ LV4 → ESP D5
- [ ] Spa Pin 7 (DSP Clock) → Shifter HV5 ↔ LV5 → ESP D6
- [ ] Spa Pin 8 (DSP CS) → Shifter HV6 ↔ LV6 → ESP D7

## Visual Reference for D1 Mini Pins

```
      [USB Port]
         ___
        |USB|
        |___|

   RST  [●]  TX
   A0   [●]  RX
   D0   [●]  D1  ← CIO Data (GPIO5)
   D5   [●]  D2  ← CIO Clock (GPIO4)
   D6   [●]  D3  ← CIO CS (GPIO0)
   D7   [●]  D4
   D8   [●]  GND
   3V3  [●]  5V
```

**DSP Pins (other side):**
- D5 = GPIO14 (DSP Data)
- D6 = GPIO12 (DSP Clock)
- D7 = GPIO13 (DSP CS)

## How to Test Your Wiring

### Step 1: Visual Inspection
1. Check all connections are firm (no loose dupont connectors)
2. Verify no crossed wires
3. Ensure correct orientation of level shifter (LV to ESP, HV to spa)

### Step 2: Multimeter Test (Power Off!)
With spa AND ESP8266 unplugged:

1. **Continuity test** - Place multimeter in continuity mode:
   - Touch Spa Pin 1 and Level Shifter HV → Should beep
   - Touch Spa Pin 2 and ESP8266 GND → Should beep
   - Touch Spa Pin 3 and ESP8266 D1 → Should beep (through shifter)
   - Repeat for all signal pins

2. **No shorts test**:
   - Test 5V to GND → Should NOT beep
   - Test each signal pin to GND → Should NOT beep
   - Test each signal pin to 5V → Should NOT beep

### Step 3: Voltage Test (Power On - No Load)
Power on spa pump (ESP not connected yet):

1. Measure voltage at spa connector:
   - Pin 1 to Pin 2 → Should read 4.75V - 5.25V

2. If voltage is good, connect ESP8266 (via USB power)

3. Measure at ESP8266:
   - 3.3V pin → Should read 3.2V - 3.4V
   - 5V pin → Should read 4.75V - 5.25V

### Step 4: Flash Diagnostic Code
```bash
esphome run spa_diagnostic.yaml
```

Watch for:
- WiFi connection success
- "Waiting for spa data..." messages
- Pin state logging

### Step 5: Monitor Communication
With spa powered on and running:

```bash
esphome logs spa_diagnostic.yaml
```

Look for:
- `[I] DSP CS activated - pump is sending data`
- Packet count increasing
- Pin states changing

## What If My Wiring Is Different?

### Scenario 1: I Have a 4-Wire Model
Your spa has only 4 wires instead of 8. Edit `spa_diagnostic.yaml`:

1. Find the "ALTERNATIVE: For 4-WIRE MODELS" section (lines 37-52)
2. Comment out the 6-wire pins (lines 30-35)
3. Uncomment the 4-wire pins (lines 50-52)

### Scenario 2: I Used Different ESP8266 Pins
If you wired to different pins, update the substitutions in `spa_diagnostic.yaml`:

Example - if you connected DSP Data to D4 instead of D5:
```yaml
dsp_data_pin: GPIO2  # D4 instead of GPIO14 (D5)
```

### Scenario 3: I Don't Have a Level Shifter
Some setups work without a level shifter (though not recommended):
- Your wiring goes directly from spa to ESP8266
- This is risky but some users report it works
- The pin mappings stay the same, just without the shifter in between

## Supported Models Reference

Your configuration is set for **6-wire 2021 models**, which includes:

- ✅ Bestway Paris (NO54173) - 2021
- ✅ Bestway Helsinki (NO54144)
- ✅ Bestway Palm Springs (NO54148)
- ✅ Bestway Monaco (NO54150)

If your model number is different, double-check if it's 4-wire or 6-wire:
- Count the wires in the cable between pump and display
- 6-8 wires = 6-wire model (current config is correct)
- 4 wires = 4-wire model (need to change config)

## Need to Change Model Type?

If you discover you have a different model:

1. Edit `spa_diagnostic.yaml` - change pin configuration
2. Edit `bestway_lazyspa_advanced.yaml` line 11:
   ```yaml
   model_type: "6wire_2021"  # Change to match your model
   ```

Options:
- `6wire_2021` - Current setting
- `6wire_pre2021` - Older 6-wire models
- `4wire_2021` - Newer 4-wire models
- `4wire_pre2021` - Older 4-wire models

## Ready to Test?

If you've verified your wiring matches the diagram above, you're ready to flash:

```bash
esphome run spa_diagnostic.yaml
```

The pins are already configured correctly for a 6-wire 2021 model!
