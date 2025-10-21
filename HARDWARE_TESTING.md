# Hardware Testing Procedures

## Purpose

This guide helps you determine if your hardware setup is correct AFTER fixing the code issues documented in `DEBUGGING_GUIDE.md`.

## Prerequisites

1. Code issues have been fixed (see DEBUGGING_GUIDE.md)
2. ESP8266 can compile and run basic code
3. You have the diagnostic YAML flashed: `spa_diagnostic.yaml`

## Safety First

⚠️ **WARNING:**
- Disconnect spa from mains power before opening
- Never work on wiring while spa is powered
- Use GFCI/RCD protection on all electrical connections
- Keep ESP8266 away from water
- Double-check all connections before applying power

## Testing Sequence

### Phase 1: Visual Inspection

**Before any power:**

1. **Check Wire Connections**
   ```
   ✓ All wires firmly connected (no loose dupont connectors)
   ✓ No bare wire exposed (risk of shorts)
   ✓ Correct wire gauge (22-26 AWG recommended)
   ✓ Wires under 30cm length for signal integrity
   ✓ No parallel runs with AC power cables
   ```

2. **Verify Pin Mapping**
   ```
   Spa Connector -> Level Shifter -> ESP8266

   For 6-wire models:
   Pin 1 (5V)     -> HV          -> 5V/VIN
   Pin 2 (GND)    -> GND         -> GND
   Pin 3 (CIO_D)  -> HV1<->LV1   -> D1 (GPIO5)
   Pin 4 (CIO_C)  -> HV2<->LV2   -> D2 (GPIO4)
   Pin 5 (CIO_CS) -> HV3<->LV3   -> D3 (GPIO0)
   Pin 6 (DSP_D)  -> HV4<->LV4   -> D5 (GPIO14)
   Pin 7 (DSP_C)  -> HV5<->LV5   -> D6 (GPIO12)
   Pin 8 (DSP_CS) -> HV6<->LV6   -> D7 (GPIO13)

   For 4-wire models:
   Pin 1 (5V)     -> 5V/VIN
   Pin 2 (GND)    -> GND
   Pin 3 (DSP_TX) -> D5 (GPIO14)
   Pin 4 (CIO_TX) -> D7 (GPIO13)
   ```

3. **Inspect Level Shifter (6-wire only)**
   ```
   ✓ LV (low voltage) connected to ESP8266 3.3V
   ✓ HV (high voltage) connected to spa 5V
   ✓ Common ground connected
   ✓ No reversed connections
   ✓ Shifter powered before signal pins used
   ```

---

### Phase 2: Power Testing

**Equipment needed:**
- Multimeter
- Spa pump (powered from mains, but ESP NOT connected yet)

**Test 1: Spa Power Output**

1. Power on spa pump (ESP8266 NOT connected)
2. Measure voltage at spa connector:
   ```
   Expected: 4.75V - 5.25V between power and ground pins
   Actual: _____ V

   ✓ Pass: 4.75-5.25V
   ✗ Fail: <4.5V or >5.5V - DO NOT PROCEED
   ```

**Test 2: ESP8266 Power**

1. Connect ESP8266 to USB power ONLY (not to spa)
2. Measure voltage:
   ```
   ESP8266 3.3V pin: Expected 3.2-3.4V, Actual: _____ V
   ESP8266 5V pin:   Expected 4.75-5.25V, Actual: _____ V

   ✓ Pass: Both within range
   ✗ Fail: Check USB power supply quality
   ```

**Test 3: Level Shifter Power (6-wire only)**

1. Connect level shifter to ESP8266 and spa power
2. Measure voltages:
   ```
   LV (low voltage side): Expected 3.2-3.4V, Actual: _____ V
   HV (high voltage side): Expected 4.75-5.25V, Actual: _____ V

   ✓ Pass: Both correct
   ✗ Fail: Check connections and level shifter orientation
   ```

---

### Phase 3: Signal Testing

**Equipment needed:**
- Logic analyzer OR oscilloscope (recommended)
- OR multimeter in DC voltage mode (basic test)

**Test 1: Signal Presence (Multimeter Method)**

1. Connect everything (ESP8266 powered via USB, spa powered and connected)
2. Measure voltage on signal pins while spa is idle:
   ```
   DSP_DATA (D5):  _____ V (should be 3-5V when idle, 0V when active)
   DSP_CLK (D6):   _____ V (should toggle between 0-5V)
   DSP_CS (D7):    _____ V (should be 3-5V when idle, 0V when transmitting)
   ```

3. Turn on spa pump filter/heater and measure again:
   ```
   Do signals change? Yes / No

   ✓ Pass: Signals change when spa does something
   ✗ Fail: Signals stuck at same voltage
   ```

**Test 2: Signal Levels (Logic Analyzer Method)**

1. Connect logic analyzer to DSP pins
2. Set trigger on DSP_CS falling edge
3. Power on spa and wait for transmission
4. Capture data

**Expected waveform:**
```
CS:   ‾‾‾‾\_______________________/‾‾‾‾
CLK:  ______/‾\_/‾\_/‾\_/‾\_/‾\_/‾\____
DATA: ____/‾‾‾\___/‾‾‾‾‾‾\_________...

       ^     ^
       |     |
       Start of transmission
```

**What to look for:**
```
✓ CS goes low (0V) at start of transmission
✓ Clock pulses appear (square wave ~10-100kHz)
✓ Data line changes between clock pulses
✓ Transmission completes (CS goes high again)
✓ Voltage levels: 0V = low, 3-5V = high

✗ No clock pulses - check clock wire connection
✗ Clock present but no data - check data wire
✗ Voltage wrong (e.g., 1.5V = high) - check level shifter
```

---

### Phase 4: ESP8266 Software Test

**Prerequisites:**
- `spa_diagnostic.yaml` flashed to ESP8266
- All hardware connected
- Spa powered on

**Test 1: Boot and WiFi**

1. Flash diagnostic config:
   ```bash
   esphome run spa_diagnostic.yaml
   ```

2. Watch serial output:
   ```
   Expected output:
   [I][app:100] ESPHome version...
   [C][wifi:369] WiFi:
   [C][wifi:384]   SSID: 'YourWiFi'
   [I][wifi:580] WiFi Connected!

   ✓ Pass: Connects to WiFi
   ✗ Fail: Check WiFi credentials in secrets.yaml
   ```

**Test 2: Pin State Monitoring**

1. Open logs:
   ```bash
   esphome logs spa_diagnostic.yaml
   ```

2. Look for pin state messages:
   ```
   [D][pins:XXX] Pin states - CS:0 CLK:0 DATA:0

   ✓ Pass: Pin states shown
   ✗ Fail: ESP8266 not reading pins
   ```

3. Turn on spa heater/filter and watch logs:
   ```
   [I][diagnostic:XXX] DSP CS activated - pump is sending data
   [I][diagnostic:XXX] DSP CS deactivated

   ✓ Pass: CS activation detected
   ✗ Fail: No CS activation = wiring problem
   ```

**Test 3: Packet Reception**

1. Check "DSP Packet Count" sensor in Home Assistant or web UI
2. Power on spa and wait 30 seconds
3. Check packet count

   ```
   Packet count after 30s: _____

   ✓ Pass: Count > 0 (hardware working!)
   ✗ Fail: Count = 0 (see troubleshooting below)
   ```

**Test 4: Communication Quality**

1. Let diagnostic run for 5 minutes
2. Check logs for warnings:
   ```
   [I][diagnostic:XXX] Communication OK - received XX packets in last 30s

   ✓ Pass: Regular "Communication OK" messages
   ✗ Fail: "No packets received" warnings
   ```

---

### Phase 5: Troubleshooting Failed Tests

#### Problem: No Packets Received (count = 0)

**Possible causes:**

1. **Wrong pin mapping**
   ```
   Fix: Double-check pin assignments match your wiring
   Try alternative pin configuration from wiring.md
   ```

2. **Level shifter not working (6-wire)**
   ```
   Test: Remove level shifter, connect signals directly (TEMPORARY TEST ONLY)
   WARNING: May damage ESP8266 - only for quick test

   If works without shifter: Shifter is bad or wrong type
   If still fails: Not a shifter issue
   ```

3. **Spa not transmitting**
   ```
   Test: Verify spa display works when ESP disconnected
   Some spas only transmit when buttons pressed - try pressing buttons
   ```

4. **Inverted chip select**
   ```
   Fix: Try inverting CS in diagnostic YAML:

   pin:
     number: ${dsp_cs_pin}
     mode: INPUT
     inverted: false  # Change to false (was true)
   ```

5. **Noise/interference**
   ```
   Fix:
   - Shorten wires to <20cm
   - Add 100nF capacitor between data line and ground
   - Separate signal wires from AC power cables
   - Use shielded cable
   ```

#### Problem: Packets Received but Intermittent

**Possible causes:**

1. **Loose connections**
   ```
   Fix: Solder connections instead of dupont wires
   Check for cold solder joints
   ```

2. **Wire too long**
   ```
   Fix: Keep under 30cm, ideally under 20cm
   ```

3. **Power supply noise**
   ```
   Fix: Add 100µF capacitor across ESP8266 power pins
   Use separate power supply instead of USB
   ```

4. **EMI from spa pump motor**
   ```
   Fix: Add ferrite beads to signal wires
   Move ESP8266 away from pump motor
   ```

---

## Test Results Template

Use this template to document your testing:

```
=== HARDWARE TEST RESULTS ===
Date: __________
Spa Model: __________
ESP8266 Board: __________

PHASE 1 - VISUAL INSPECTION
[  ] All connections verified
[  ] Pin mapping correct
[  ] Level shifter oriented correctly
Notes: __________

PHASE 2 - POWER TESTING
Spa output voltage: _____ V
ESP 3.3V rail: _____ V
ESP 5V rail: _____ V
[  ] All voltages in spec

PHASE 3 - SIGNAL TESTING
[  ] DSP_CS toggles
[  ] DSP_CLK pulses during transmission
[  ] DSP_DATA changes
Logic analyzer attached: Yes / No
Notes: __________

PHASE 4 - SOFTWARE TESTING
WiFi connected: Yes / No
DSP packet count after 5 min: _____
[  ] Packets received
Communication quality: _____ %

CONCLUSION
[  ] Hardware working - proceed to advanced config
[  ] Hardware issue - see troubleshooting
Issue description: __________
```

---

## Success Criteria

Your hardware is confirmed working if:

✅ DSP packet count increases over time
✅ "Communication OK" messages in logs
✅ Pin states change when spa operates
✅ No "WARNING: No packets" messages after spa is powered for 2+ minutes

If all above are true: **Hardware is OK! The issue was in the code, which is now fixed.**

Proceed to flash the advanced configuration.

---

## Next Steps After Successful Hardware Test

1. Keep diagnostic YAML as a backup
2. Flash the corrected advanced YAML (once GPIO issues are fully fixed)
3. Monitor logs for any protocol errors
4. Gradually enable features (heater, filter, bubbles, etc.)
5. Test integration with Home Assistant

---

## When to Suspect Hardware Issues

Hardware is likely faulty if:

❌ No DSP packets after 10 minutes with spa running
❌ Logic analyzer shows no signals on any pin
❌ Voltages out of spec in Phase 2 tests
❌ Direct connection (bypassing level shifter) also fails
❌ Multiple ESP8266 boards fail identically

In these cases:
1. Recheck all wiring with continuity tester
2. Try different cables
3. Test with known-good level shifter
4. Verify spa model compatibility (see README.md)

---

## Support

If hardware testing fails after following all troubleshooting steps:

1. Document your test results using template above
2. Take clear photos of wiring
3. Capture logic analyzer screenshots if available
4. Post in GitHub discussions with all details

**Remember:** In most cases, if the diagnostic shows packet count increasing, your hardware is working correctly!
