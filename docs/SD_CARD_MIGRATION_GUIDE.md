# SD Card Migration Guide

## What Changed

Your FPVGate now uses an **SD card** to store audio files, freeing up ESP32 internal flash memory. Here's what happened:

### Storage Layout (New)
- **LittleFS (ESP32 Flash)** - 1 MB
  - Web interface files (HTML, JS, CSS)
  - Configuration and race history
  
- **SD Card** - All your card capacity (e.g. 32GB)
  - Audio files (`/sounds` directory - 1.7 MB of MP3 files)
  - Future expansion room

### Benefits
✅ **85% more free space** on ESP32 flash  
✅ **Larger OTA updates** possible (2MB firmware vs 1.8MB before)  
✅ **Add unlimited sounds** without reflashing  
✅ **Graceful fallback** - if SD fails, system still works from LittleFS  

## Current Status

✅ **SD card is working!** Your hardware detected:
- Type: SDHC/SDXC
- Size: 30GB
- Pin Configuration: CS=39, SCK=36, MOSI=35, MISO=37

❌ **Sounds not migrated yet** - They're still in LittleFS, need to be copied to SD

## Next Steps

### Option 1: Manual Copy via Computer (Easiest)

1. **Remove SD card** from ESP32
2. **Insert into computer** (use SD card reader)
3. **Copy sounds** from `data/sounds/` directory to SD card root:
   ```
   SD:\
   └── sounds\
       ├── arm_your_quad.mp3
       ├── lap_1.mp3
       ├── lap_2.mp3
       └── ... (all 125+ MP3 files)
   ```
4. **Eject SD card** and reinsert into ESP32
5. **Power cycle** ESP32
6. **Test audio** - visit web interface and trigger a lap

### Option 2: Via ESP32 (Automatic Migration)

The firmware includes automatic migration that copies sounds from LittleFS to SD card **5 seconds after boot**.

**Current issue:** LittleFS has web files but NO sounds (we uploaded without sounds to save space).

**Solution:**
1. Re-upload filesystem WITH sounds:
   ```powershell
   python -m platformio run -e ESP32S3 -t uploadfs --upload-port COM12
   ```
   
2. **Reboot** ESP32

3. **Monitor serial** output - should see:
   ```
   === Deferred SD card initialization ===
   ✅ SD card mounted successfully
   === Migrating sounds to SD card ===
   Copying /sounds to SD card...
   Copied 125 files (1690 KB) with 0 errors
   ✅ Migration complete!
   ```

4. **(Optional) Free up LittleFS space** by deleting sounds from flash:
   - Upload minimal filesystem again (without sounds)
   - OR keep them as backup (costs 1.7MB flash)

## Testing

1. **Connect to FPVGate WiFi**
2. **Open web interface** (http://192.168.4.1)
3. **Go to Configuration tab**
4. **Test voice** using "Test Voice" button
5. **Start a race** and trigger laps to hear announcements

## Troubleshooting

### SD Card Not Detected
- Check wiring (see pin configuration above)
- Try different SD card (must be FAT32 formatted)
- Check serial monitor for error messages
- Some cards are incompatible - try name-brand SD/SDHC card

### Audio Files Not Playing
1. Check SD card has `/sounds` directory
2. Verify MP3 files are present
3. Check serial monitor - should show "Serving audio from SD: /sounds/..."
4. If showing "Serving audio from LittleFS", SD card not detected

### Web Interface Not Loading
- LittleFS must have web files (index.html, script.js, etc.)
- SD card failure won't affect web interface
- Re-upload filesystem if needed

## Technical Details

### Pin Configuration
```
ESP32-S3          SD Card
GPIO39 (CS)   →   CS (Chip Select)
GPIO36 (SCK)  →   SCK (Clock)
GPIO35 (MOSI) →   MOSI (Data In)
GPIO37 (MISO) ←   MISO (Data Out)
GND           →   GND
3.3V          →   VCC
```

### Partition Table Changes
```
Old (4.4MB LittleFS):
app0:    0x10000  - 0x1D0000  (1.8MB)
app1:    0x1D0000 - 0x390000  (1.8MB)
spiffs:  0x390000 - 0x800000  (4.4MB)

New (1MB LittleFS):
app0:    0x10000  - 0x210000  (2MB)
app1:    0x210000 - 0x410000  (2MB)
spiffs:  0x410000 - 0x510000  (1MB)
```

### File Serving Priority
1. Try **SD card** first (if available)
2. Fall back to **LittleFS** (always works)
3. Return **404** if not found in either

### Migration Logic
- Runs once, 5 seconds after boot
- Checks if `/sounds` exists on SD
- If not, copies from LittleFS → SD
- Non-destructive (doesn't delete from LittleFS automatically)

## Future Enhancements

With SD card working, you can now:
- Add more voices without reflashing
- Store race telemetry data
- Log RSSI graphs for analysis
- Add firmware update files for offline OTA

## Questions?

Check the serial monitor output for detailed debug info:
```powershell
python -m platformio device monitor -e ESP32S3 --port COM12
```

Look for lines starting with:
- `=== SD Card Initialization ===`
- `=== Migrating sounds to SD card ===`
- `Serving audio from SD: ...`
