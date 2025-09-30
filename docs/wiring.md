# Wiring Guide for ESPHome Bestway Spa Controller

## ⚠️ Safety First

**WARNING**: 
- **ALWAYS** disconnect the spa from mains power before opening
- Water and electricity are lethal - take proper precautions
- This modification will void your warranty
- If you're not comfortable with electronics, seek help

## Required Components

### For All Models
- ESP8266 board (Wemos D1 Mini recommended)
- Dupont wires or ribbon cable
- Heat shrink tubing or electrical tape
- Small project box (optional but recommended)

### For 6-Wire Models
- Bidirectional logic level shifter (3.3V ↔ 5V)
  - Recommended: TXS0108E or similar
  - Alternative: 4-channel bidirectional shifter

### For 4-Wire Models
- Level shifter optional (many work without it)
- If display flashes: Add 510Ω pull-up resistors

## Pin Identification

### 6-Wire Connector Pinout

```
    Pump Side Connector (Female)
    ┌─────────────────────────┐
    │ 1   2   3   4   5   6   │
    │ O   O   O   O   O   O   │
    └─────────────────────────┘
    
Pin 1: 5V Power (Usually RED)
Pin 2: Ground (Usually BLACK)
Pin 3: CIO Data (Usually YELLOW)
Pin 4: CIO Clock (Usually GREEN)
Pin 5: CIO Chip Select (Usually BLUE)
Pin 6: DSP Data (Usually WHITE)
```

Additional pins on some models:
- Pin 7: DSP Clock
- Pin 8: DSP Chip Select

### 4-Wire Connector Pinout

```
    Pump Side Connector (Female)
    ┌─────────────────┐
    │ 1   2   3   4   │
    │ O   O   O   O   │
    └─────────────────┘
    
Pin 1: 5V Power (Usually BLACK)
Pin 2: Ground (Usually RED)
Pin 3: DSP TX (Usually YELLOW)
Pin 4: CIO TX (Usually GREEN)
```

**Note**: Wire colors vary by manufacturer! Always verify with multimeter.

## Wiring Diagrams

### 6-Wire Model with Level Shifter

```
PUMP                  LEVEL SHIFTER              ESP8266 (D1 Mini)
────                  ─────────────              ─────────────────
5V    ───────────────> HV, VA ──────────────────> 5V/VIN
GND   ───────────────> GND ─────────────────────> GND
                      │ LV, VB <─────────────────< 3.3V
                      │
CIO_DATA <──────────> HV1 <──> LV1 <────────────> D1 (GPIO5)
CIO_CLK  <──────────> HV2 <──> LV2 <────────────> D2 (GPIO4)
CIO_CS   <──────────> HV3 <──> LV3 <────────────> D3 (GPIO0)
DSP_DATA ──────────> HV4 <──> LV4 <─────────────> D5 (GPIO14)
DSP_CLK  ──────────> HV5 <──> LV5 <─────────────> D6 (GPIO12)
DSP_CS   ──────────> HV6 <──> LV6 <─────────────> D7 (GPIO13)
```

### 4-Wire Model (Direct Connection)

```
PUMP                           ESP8266 (D1 Mini)
────                           ─────────────────
5V    ────────────────────────> 5V/VIN
GND   ────────────────────────> GND
DSP_TX ───────────────────────> D5 (GPIO14)
CIO_TX ───────────────────────> D7 (GPIO13)
```

### 4-Wire Model with Pull-up Resistors (if needed)

```
PUMP                           ESP8266 (D1 Mini)
────                           ─────────────────
5V    ────────────────────────> 5V/VIN
      │                        
      ├──[510Ω]──┐            
      │          │            
GND   ───────────┼────────────> GND
                 │            
DSP_TX ──────────┴────────────> D5 (GPIO14)
      │                        
      ├──[510Ω]──┐            
      │          │            
CIO_TX ──────────┴────────────> D7 (GPIO13)
```

## Step-by-Step Installation

### 1. Prepare the Hardware

1. **Solder headers** to ESP8266 if needed
2. **Prepare wires**:
   - Cut to appropriate length (keep under 30cm)
   - Strip ends carefully
   - Tin wire ends for better connection

### 2. Connect Level Shifter (6-wire only)

1. Connect power:
   - 5V to HV/VA
   - 3.3V from ESP to LV/VB
   - Common ground to GND

2. Connect signal lines according to diagram

### 3. Make the Connections

#### Method A: Intercept Cable (Recommended)
1. Create a Y-cable or intercept board
2. Pump connects to one side
3. Display connects to other side
4. ESP8266 taps into signals

#### Method B: Direct Replacement
1. Disconnect display completely
2. ESP8266 takes place of display
3. Display functions via web interface

### 4. Test Before Final Assembly

1. **Power on** with pump disconnected from mains
2. **Check voltages** with multimeter
3. **Flash ESP8266** with test firmware
4. **Verify communication** via serial monitor

### 5. Final Installation

1. **Mount ESP8266** in project box
2. **Secure all connections** with hot glue or tape
3. **Route cables** away from high voltage
4. **Test with pump** connected to mains

## Alternative Pin Configurations

If default pins cause issues, try these alternatives:

### Alternative 1 (Better for 4-wire)
```yaml
substitutions:
  dsp_data_pin: GPIO2   # D4
  dsp_clk_pin: GPIO14   # D5
  dsp_cs_pin: GPIO12    # D6
```

### Alternative 2 (Avoid boot pins)
```yaml
substitutions:
  cio_data_pin: GPIO13  # D7
  cio_clk_pin: GPIO12   # D6
  cio_cs_pin: GPIO14    # D5
  dsp_data_pin: GPIO5   # D1
  dsp_clk_pin: GPIO4    # D2
  dsp_cs_pin: GPIO2     # D4
```

## Troubleshooting Wiring Issues

### Display shows "---" or "888"
- Check DSP connections
- Verify level shifter is powered correctly
- Try adding pull-up resistors

### Display flashes or flickers
- Add 510Ω pull-up resistors on data lines
- Check for loose connections
- Reduce wire length

### No communication
- Verify 5V and GND connections
- Check level shifter orientation
- Test with multimeter for continuity

### ESP8266 won't boot
- GPIO0 or GPIO2 may be pulled low
- Try alternative pin configuration
- Disconnect and reconnect after boot

### Intermittent connection
- Use shorter wires
- Add ferrite beads for noise suppression
- Ensure solid connections (solder better than breadboard)

## Tips for Success

1. **Use quality wires** - Silicone insulated recommended
2. **Keep wires short** - Under 30cm for best results
3. **Avoid parallel runs** with AC power cables
4. **Test incrementally** - Don't connect everything at once
5. **Document your setup** - Take photos before and after
6. **Have a multimeter ready** - Essential for troubleshooting

## Model-Specific Notes

### Bestway Paris (NO54173)
- Confirmed working with standard 6-wire setup
- No pull-ups needed

### Bestway Helsinki (NO54144)
- May need stronger pull-ups (330Ω)
- Some units have reversed CIO/DSP pins

### 4-Wire Models (NO54138, etc)
- Often work without level shifter
- Filter status may not report correctly
- Temperature reading sometimes delayed

## Need Help?

If you're having wiring issues:
1. Double-check all connections with multimeter
2. Verify correct pin mapping in configuration
3. Try alternative pin configurations
4. Post clear photos in GitHub discussions

Remember: Every spa model can be slightly different. What works for one might need adjustment for another.