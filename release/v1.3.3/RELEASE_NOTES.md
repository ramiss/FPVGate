# FPVGate v1.3.3 Release Notes

**Release Date:** December 10, 2024

## What's New

### Modern Configuration Interface
The configuration menu has been completely redesigned with a modern, full-screen overlay modal inspired by Mainsail's Interface settings.

**Key Features:**
- **Sidebar Navigation** - Easy-to-navigate sections with clear visual hierarchy
- **6 Organized Sections:**
  - **Lap & Announcer Settings** - Race setup and TTS controls (merged from General + TTS)
  - **Pilot Info** - Band, channel, name, callsign, phonetic name, and color
  - **LED Setup** - Presets, colors, animation speed, brightness, manual override
  - **WiFi & Connection** - Connection mode, COM port, WiFi SSID/password with Apply/Reset
  - **System Settings** - Battery monitoring, theme, OBS integration, firmware update
  - **Diagnostics** - System self-test
- **Mobile Responsive** - Sidebar converts to horizontal tabs on small screens
- **Intuitive Controls** - Click outside or press ESC to close, settings footer with Save/Download/Import
- **Cleaner Design** - Removed emojis, improved spacing, better visual feedback

### WebSocket Stability Improvements
- **SSE Keepalive** - Automatic ping every 15 seconds prevents connection timeout
- **Fixes RSSI Disconnection** - No more page refreshes required to restore RSSI updates
- **Stable Long-Running Sessions** - Reliable connections for extended racing sessions

### Bug Fixes
- **Duplicate Element IDs** - Resolved JavaScript targeting issues by removing old config tab
- **Battery Monitor Toggle** - Now properly shows/hides battery monitoring section
- **LED Animation Speed** - Correctly hidden for Off, Solid Colour, and Pilot Colour presets
- **WiFi Configuration** - Fixed non-functional Apply/Reset buttons, proper restart warnings
- **UI Rendering** - Removed stray `` `n `` characters appearing in HTML output

## Installation

### ESP32-S3 (8MB Flash)

**Required Files:**
- `FPVGate_v1.3.3_ESP32S3_bootloader.bin` (at 0x0000)
- `FPVGate_v1.3.3_ESP32S3_partitions.bin` (at 0x8000)
- `FPVGate_v1.3.3_ESP32S3_firmware.bin` (at 0x10000)
- `FPVGate_v1.3.3_ESP32S3_littlefs.bin` (at 0x410000)

**Flash Command:**
```bash
esptool.py --chip esp32s3 --port COM# write_flash 0x0000 FPVGate_v1.3.3_ESP32S3_bootloader.bin 0x8000 FPVGate_v1.3.3_ESP32S3_partitions.bin 0x10000 FPVGate_v1.3.3_ESP32S3_firmware.bin 0x410000 FPVGate_v1.3.3_ESP32S3_littlefs.bin
```

**PlatformIO:**
```bash
pio run -e ESP32S3 --target upload --upload-port COM#
pio run -e ESP32S3 --target uploadfs --upload-port COM#
```

### Upgrading from v1.3.2
Since only web interface files changed, you can:
1. Upload just the filesystem: `pio run -e ESP32S3 --target uploadfs`
2. Or use OTA update via web interface at `/update`
3. Full flash is recommended for clean installation

## Compatibility
- **ESP32-S3 DevKitC-1** (8MB Flash) - Primary target
- **ESP32-C3** - Compatible (not tested this release)
- **ESP32** - Compatible (not tested this release)

## Known Issues
None reported. This is a stable UI improvement release.

## Technical Details

### Files Modified
- `data/index.html` - Settings modal structure with sidebar navigation
- `data/style.css` - Complete modal styling system (lines 934-1418)
- `data/script.js` - Modal management functions and auto-save
- `lib/WEBSERVER/webserver.h` - SSE keepalive timer constant
- `lib/WEBSERVER/webserver.cpp` - SSE keepalive ping implementation

### Memory Usage (ESP32-S3)
- **RAM:** 22.9% (75,136 / 327,680 bytes)
- **Flash:** 50.0% (1,048,005 / 2,097,152 bytes)

## Support
- **Documentation:** https://github.com/LouisHitchcock/FPVGate/wiki
- **Issues:** https://github.com/LouisHitchcock/FPVGate/issues
- **Discussions:** https://github.com/LouisHitchcock/FPVGate/discussions

## Credits
Special thanks to the FPV community for feedback on the configuration interface!

---

**Full Changelog:** https://github.com/LouisHitchcock/FPVGate/blob/main/CHANGELOG.md
