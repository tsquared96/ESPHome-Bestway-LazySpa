# Debugging Guide - Code vs Hardware Issues

## Executive Summary

**STATUS: CODE ISSUES FOUND** - Multiple critical code errors identified that would prevent compilation and operation.

## Critical Code Issues

### 1. GPIO Pin Initialization Error (CRITICAL)

**Location:** `bestway_lazyspa_advanced.yaml` lines 200-210, `config/advanced.yaml` lines 200-210

**Problem:**
```cpp
cio_data = new GPIOPin(${cio_data_pin}, OUTPUT);  // WRONG!
```

**Why It Fails:**
- ESPHome's `GPIOPin` class cannot be instantiated this way in lambda code
- The substitution `${cio_data_pin}` expands to `GPIO5` which is not a valid constructor parameter
- This code will NOT compile

**Symptoms:**
- Compilation fails with errors about GPIOPin constructor
- If it somehow compiles, runtime crashes on pin access

**Fix Required:**
Use ESPHome's proper pin API within lambdas. See fixes below.

---

### 2. Include Path Error

**Location:** `bestway_lazyspa_advanced.yaml` line 41

**Problem:**
```yaml
includes:
  - spa_protocol.h  # WRONG - file doesn't exist at this path
```

**Correct Path:**
```yaml
includes:
  - includes/spa_protocol.h  # Correct path
```

**Why It Fails:**
- ESPHome looks for includes relative to the YAML file
- The file is in `includes/` subdirectory
- Compilation will fail with "file not found"

---

### 3. Custom Component Structure Issues

**Location:** Lines 179-309

**Problems:**
1. **Incorrect inheritance:**
   ```cpp
   class BestwaySpaCommunication : public PollingComponent, public Sensor
   ```
   - Should only inherit from `PollingComponent`
   - Inheriting from `Sensor` requires different implementation

2. **Pin API misuse:**
   - `digital_read()` and `digital_write()` methods don't exist on GPIOPin objects in this context
   - Need to use ESPHome's App global pin access

3. **No timeout protection:**
   ```cpp
   while (!dsp_clk->digital_read()) { yield(); }
   ```
   - Can hang forever if hardware doesn't respond
   - Will cause watchdog resets

---

### 4. Component Access Error

**Location:** Lines 335, 343, 365, 516, etc.

**Problem:**
```yaml
lambda: |-
  id(spa_communication).send_button_press(0xFF);  # WRONG!
```

**Why It Fails:**
- Custom component registered as ID but methods not properly exposed
- The component needs to be accessed differently in ESPHome

---

## How to Diagnose: Code vs Hardware

### Step 1: Try to Compile

```bash
esphome compile bestway_lazyspa_advanced.yaml
```

**Expected Result with Current Code:** COMPILATION FAILS

**If compilation fails:** Code issue (confirmed)
**If compilation succeeds:** Move to Step 2

### Step 2: Check Logs During Boot

Flash the device and watch serial output:

```bash
esphome logs bestway_lazyspa_advanced.yaml
```

**Look for:**
- `SPA communication initialized` - If missing, setup() failed
- `Starting advanced SPA initialization` - Boot script running
- Crash/restart loops - Memory or pointer issues

### Step 3: Hardware Testing (Only After Code Fixes)

Once code compiles and runs, test hardware:

1. **Power Test:**
   ```
   - Check 5V and GND with multimeter
   - Should read 4.75V - 5.25V
   ```

2. **Signal Test:**
   ```
   - Check DSP_DATA, DSP_CLK, DSP_CS with logic analyzer or oscilloscope
   - Should see clock pulses when spa is powered
   ```

3. **Communication Test:**
   ```
   - Check logs for "Comm Quality" sensor
   - Should be >80% if working
   ```

---

## Quick Test Configuration

Here's a minimal test version to verify hardware connections:

### Minimal Test YAML (Hardware Verification)

```yaml
esphome:
  name: spa-hardware-test
  platform: ESP8266
  board: d1_mini

wifi:
  ssid: "your_wifi"
  password: "your_password"

logger:
  level: DEBUG

api:
ota:

# Simple pin state monitoring
binary_sensor:
  - platform: gpio
    pin:
      number: GPIO14  # DSP_DATA
      mode: INPUT
    name: "DSP Data Pin"

  - platform: gpio
    pin:
      number: GPIO12  # DSP_CLK
      mode: INPUT
    name: "DSP Clock Pin"

  - platform: gpio
    pin:
      number: GPIO13  # DSP_CS
      mode: INPUT
    name: "DSP CS Pin"
```

**How to Use:**
1. Flash this minimal config
2. Watch the binary sensors in Home Assistant or logs
3. Turn on spa pump
4. **If pins change state:** Hardware is good, original code is the problem
5. **If pins stay static:** Check wiring

---

## Recommended Fix Strategy

### Option A: Complete Rewrite (Recommended)

The custom component approach has too many issues. Better approach:

1. Use ESPHome's built-in `custom_component` or external component
2. Use proper ESPHome pin APIs
3. Separate protocol handling into dedicated C++ component
4. Use ESPHome's SPI component instead of bit-banging

### Option B: Minimal Fixes (Quick)

Fix just enough to get it working:

1. Fix include path
2. Remove complex custom component
3. Use simple GPIO polling in lambdas
4. Add debug logging at every step

---

## Next Steps

1. **Create fixed version of the YAML** with corrected pin handling
2. **Create diagnostic version** with extensive logging
3. **Test compilation** to confirm code fixes
4. **Flash and monitor** serial output
5. **Test hardware** only after software works

---

## Hardware Issues vs Code Issues - Checklist

### Definitely Code Issues (Current Status):
- ✅ GPIO pin initialization syntax
- ✅ Include file path
- ✅ Custom component structure
- ✅ Component ID access
- ✅ Missing error handling

### Possible Hardware Issues (Test After Code Fixes):
- ❓ Wiring connections
- ❓ Level shifter functionality
- ❓ Signal voltage levels
- ❓ Wire length/interference
- ❓ Spa model compatibility

---

## Conclusion

**VERDICT: CODE ISSUES - NOT HARDWARE**

The current code has multiple critical errors that prevent it from compiling or running correctly. These must be fixed before any hardware testing can be meaningful.

The good news: These are fixable software issues. Once corrected, we can determine if there are any hardware problems.
