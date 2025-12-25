# Getting Started with FPVGate

Complete guide to configuring your FPVGate lap timer.

** Navigation:** [Home](../README.md) | [User Guide](USER_GUIDE.md) | [Features](FEATURES.md)

---

## Table of Contents

1. [Initial Configuration](#initial-configuration)
2. [First Flight](#first-flight)
3. [Troubleshooting](#troubleshooting)

---

## Initial Configuration

![Calibration Wizard](../screenshots/12-12-2025/Calibration%20Wizard%20Recording%20-%2012-12-2025.png)

### Step 1: Connect to FPVGate

**Power on your FPVGate device**

**WiFi Connection (Default):**

1. Look for WiFi network: `FPVGate_XXXX` (XXXX = last 4 digits of MAC)
2. Connect using password: `fpvgate1`
3. Open browser and go to:
   - `http://www.fpvgate.xyz` (preferred)
   - `http://192.168.4.1` (fallback)

**USB Connection (Optional):**

1. Connect ESP32-S3 via USB
2. Download [Electron app](https://github.com/LouisHitchcock/FPVGate/releases)
3. Launch app and select COM port
4. All features work identically

### Step 2: Configure Your VTx Frequency

1. Navigate to **Configuration** tab
2. **Pilot Info** section:
   - **Band & Channel**: Select to match your VTx
   - Default is RaceBand 8 (5917 MHz)
   - Frequency auto-calculates

**Common Frequencies:**
- RaceBand 1-8: 5658-5917 MHz
- Fatshark 1-8: 5740-5880 MHz
- Band E (DJI): 5705-5945 MHz

3. Click **"Save Configuration"**

### Step 3: Set Pilot Information

**Pilot Info Section:**

1. **Pilot Name**: Full name (used in voice callouts)
2. **Pilot Callsign**: Short name (max 10 chars, for UI)
3. **Phonetic Name**: How TTS pronounces your name
   - Example: "Louis" ? "Louie"
   - Example: "Xavier" ? "Zavier"
4. **Pilot Color**: Choose your racing color

**TTS Settings Section:**

1. **Announcer Type**: Choose announcement style
   - `Lap Time` - Announces each lap (recommended)
   - `Beep` - Just beeps
   - `2 Consecutive Laps` - Every 2 laps
   - `3 Consecutive Laps` - Every 3 laps

2. **Voice**: Select voice engine
   - `ElevenLabs (Sarah)` - Default, energetic
   - `ElevenLabs (Rachel)` - Calm
   - `PiperTTS` - Lower latency

3. **Enable Voice**: Click to test audio

4. Click **"Save Configuration"**

### Step 4: Run System Self-Test

1. Navigate to **Configuration** tab
2. Scroll to **System Diagnostics** section
3. Click **"Run System Self-Test"**

**Verify all tests pass:**
-  RX5808 RSSI Module
-  Storage (SD card or LittleFS)
-  WiFi/USB connectivity
-  Audio/Buzzer
-  RGB LEDs (if installed)
-  Configuration values

**If tests fail**, see [Troubleshooting](#troubleshooting) section.

---

## First Flight

### Step 1: Calibrate RSSI Thresholds

** Critical: Proper calibration is essential for accurate timing**

1. **Warm up your VTx** (30 seconds minimum)
2. **Position your drone** one gate distance away (~3-6 feet)
3. Navigate to **Calibration** tab
4. Watch the **RSSI graph** - note the peak value as you move drone to gate
5. **Set Enter RSSI**: 2-5 points below observed peak
6. **Set Exit RSSI**: 8-10 points below Enter RSSI
7. Click **"Save RSSI Thresholds"**

**Good Calibration Example:**
```
Observed peak: 150
Enter RSSI: 145 (5 below peak)
Exit RSSI: 135 (10 below enter)
```

**[Detailed calibration guide ?](USER_GUIDE.md#calibration)**

### Step 2: Test Detection

1. Stay in **Calibration** tab
2. Fly your drone through the gate area
3. Watch the RSSI graph:
   - Should see single clean peak
   - Peak should cross Enter threshold
   - Should exit below Exit threshold

**If you see multiple peaks or false triggers**, see [Calibration Tips](USER_GUIDE.md#calibration-tips).

### Step 3: First Race

1. Navigate to **Race** tab
2. Click **"Start Race"**
3. Wait for countdown and start beep
4. Fly through the gate!
5. Watch lap times appear in table
6. Click **"Stop Race"** when finished

**[Complete user guide ?](USER_GUIDE.md)**

---

## Troubleshooting

### Hardware Issues

**No WiFi Network Appearing**

- Wait 15 seconds after power-on
- Check 5V power supply voltage
- Verify firmware flashed successfully
- Check Serial Monitor for error messages

**No RSSI Reading**

- Verify RX5808 is powered (5V)
- Check GPIO4 connection (RSSI pin)
- Ensure RX5808 SPI mod is correct
- Test with multimeter: RSSI pin should show ~1-3V

**RGB LEDs Not Working**

- Check 5V power to LED strip
- Verify GPIO18 data connection
- Ensure `NUM_LEDS` matches your setup
- WS2812 LEDs need 5V (not 3.3V)

**SD Card Not Detected**

- Format as FAT32
- Check wiring (use 3.3V, not 5V!)
- Verify CS pin on GPIO39
- Try different SD card (Class 10 recommended)

### Software Issues

**Can't Connect to Web Interface**

- Try both URLs: `http://www.fpvgate.xyz` and `http://192.168.4.1`
- Forget and reconnect to WiFi network
- Clear browser cache
- Try different browser

**Voice Announcements Not Working**

- Click "Enable Audio" in Configuration
- Check browser allowed audio autoplay
- Verify SD card has audio files (or will use fallback)
- Test with "Generate Audio" button

**Missing Laps**

- Lower Enter RSSI threshold by 5 points
- Ensure VTx warmed up (30 seconds)
- Check band/channel matches your VTx
- Increase minimum lap time if getting false laps

**Too Many False Laps**

- Increase Minimum Lap Time to 12-15 seconds
- Raise Exit RSSI threshold
- Move timer further from flight path
- Reduce RF interference

### Getting Help

1. **Check documentation**: [User Guide](USER_GUIDE.md) | [Hardware Guide](HARDWARE_GUIDE.md)
2. **Run self-test**: Configuration ? System Diagnostics
3. **Check Serial Monitor**: For debug messages
4. **GitHub Issues**: [Report bugs](https://github.com/LouisHitchcock/FPVGate/issues)
5. **Community**: [GitHub Discussions](https://github.com/LouisHitchcock/FPVGate/discussions)

---

![Calibration Wizard Completed](../screenshots/12-12-2025/Calibration%20Wizard%20Completed%20-%2012-12-2025.png)

## Next Steps

 **Hardware assembled and tested**  
 **Firmware flashed successfully**  
 **Initial configuration complete**  
 **First flight tested**

**Continue learning:**

 **[User Guide](USER_GUIDE.md)** - Master all features  
 **[Hardware Guide](HARDWARE_GUIDE.md)** - Advanced hardware tips  
 **[Features Guide](FEATURES.md)** - Explore capabilities  
 **[Development Guide](DEVELOPMENT.md)** - Contribute to the project

---

**Need help? [Open an issue](https://github.com/LouisHitchcock/FPVGate/issues) or [start a discussion](https://github.com/LouisHitchcock/FPVGate/discussions)!**
