# FPVGate v1.3.3 - Installation Guide

## Quick Start

### What You Need
- ESP32-S3 DevKitC-1 (8MB Flash) or compatible board
- USB cable
- esptool.py or PlatformIO

### Files in This Release
- `FPVGate_v1.3.3_ESP32S3_bootloader.bin` - Bootloader
- `FPVGate_v1.3.3_ESP32S3_partitions.bin` - Partition table
- `FPVGate_v1.3.3_ESP32S3_firmware.bin` - Main firmware
- `FPVGate_v1.3.3_ESP32S3_littlefs.bin` - Web interface files
- `RELEASE_NOTES.md` - Complete release notes

## Installation Methods

### Method 1: esptool.py (Recommended for first-time flash)

1. **Install esptool** (if not already installed):
   ```bash
   pip install esptool
   ```

2. **Find your COM port**:
   - Windows: Check Device Manager (usually COM3, COM4, etc.)
   - Linux/Mac: `ls /dev/tty*` (usually /dev/ttyUSB0 or /dev/ttyACM0)

3. **Flash all files**:
   ```bash
   esptool.py --chip esp32s3 --port COM# write_flash \
     0x0000 FPVGate_v1.3.3_ESP32S3_bootloader.bin \
     0x8000 FPVGate_v1.3.3_ESP32S3_partitions.bin \
     0x10000 FPVGate_v1.3.3_ESP32S3_firmware.bin \
     0x410000 FPVGate_v1.3.3_ESP32S3_littlefs.bin
   ```

   Replace `COM#` with your actual port (e.g., `COM12` on Windows or `/dev/ttyUSB0` on Linux)

### Method 2: PlatformIO (For developers)

1. **Clone the repository**:
   ```bash
   git clone https://github.com/YourUsername/FPVGate.git
   cd FPVGate
   git checkout v1.3.3
   ```

2. **Upload firmware**:
   ```bash
   pio run -e ESP32S3 --target upload --upload-port COM#
   ```

3. **Upload filesystem**:
   ```bash
   pio run -e ESP32S3 --target uploadfs --upload-port COM#
   ```

### Method 3: OTA Update (Upgrading from v1.3.x)

If you're already running v1.3.0, v1.3.1, or v1.3.2:

1. **Connect to your FPVGate WiFi**
2. **Navigate to** `http://192.168.4.1/update`
3. **Upload** `FPVGate_v1.3.3_ESP32S3_firmware.bin`
4. **Wait for reboot**
5. **Upload** `FPVGate_v1.3.3_ESP32S3_littlefs.bin` (optional but recommended for UI fixes)

## First-Time Setup

1. **Power on your ESP32-S3** - Wait for RGB LED to indicate startup
2. **Connect to WiFi**:
   - SSID: `FPVGate_XXXX` (XXXX = last 4 digits of MAC address)
   - Password: `fpvgate1`
3. **Open browser** to `http://192.168.4.1`
4. **Configure your settings** via the new Configuration menu (click Configuration in header)

## What's New in v1.3.3

### Modern Configuration UI
- Complete redesign with sidebar navigation
- 6 organized sections for better UX
- Mobile responsive design
- Click-outside or ESC to close

### Improved Stability
- SSE keepalive prevents RSSI disconnection
- No more page refreshes required
- Stable long-running connections

### Bug Fixes
- Fixed duplicate element IDs
- Fixed battery monitoring toggle
- Fixed WiFi configuration functionality
- Fixed UI rendering issues

See `RELEASE_NOTES.md` for complete details.

## Troubleshooting

### Flash Fails
- **Hold BOOT button** during flash initiation
- **Check COM port** is correct
- **Try lower baud rate**: Add `--baud 115200` to esptool command

### Can't Connect to WiFi
- **Check SSID**: Should be `FPVGate_XXXX`
- **Verify password**: `fpvgate1`
- **Reset device**: Power cycle the ESP32-S3
- **Check distance**: Stay within range of the device

### Web Interface Not Loading
- **Clear browser cache**: Ctrl+Shift+R (or Cmd+Shift+R on Mac)
- **Try different browser**: Chrome/Edge recommended
- **Reflash filesystem**: Use Method 1 or upload littlefs.bin via OTA

### RSSI Not Updating
- **Check RX5808 connections**: Verify wiring to GPIO pins
- **Verify channel**: Ensure band/channel matches VTX
- **Check calibration**: Run Calibration Wizard
- v1.3.3 fixes the disconnection issue with SSE keepalive

## Support

- **Documentation**: https://github.com/YourUsername/FPVGate/wiki
- **Issues**: https://github.com/YourUsername/FPVGate/issues
- **Discussions**: https://github.com/YourUsername/FPVGate/discussions

## Hardware Requirements

- **ESP32-S3 DevKitC-1** with 8MB Flash (primary target)
- **RX5808 module** for 5.8GHz video receiver
- **WS2812 RGB LED** (optional, on GPIO48)
- **Buzzer** (optional, on GPIO5)
- **SD Card** (optional, for race storage)

See full hardware guide in documentation.

---

**Release Date:** December 10, 2024  
**Version:** 1.3.3  
**License:** MIT
