================================================================================
  FPVGate v1.4.0 - Release Package
================================================================================

CONTENTS:
  - bootloader-esp32s3.bin    (15,104 bytes)
  - partitions-esp32s3.bin    (3,072 bytes)
  - firmware-esp32s3.bin      (1,228,809 bytes)
  - littlefs-esp32s3.bin      (1,048,576 bytes)
  - FPVGate_Desktop_v1.4.0_Windows_x64.zip
  - RELEASE_NOTES.md
  - README.txt (this file)

================================================================================
  WHAT'S NEW IN v1.4.0
================================================================================

  ✨ Webhook System - Send HTTP requests to external LED controllers
  ✨ Gate LED Controls - Granular control over race event webhooks
  ✨ Enhanced Self-Tests - 19 comprehensive diagnostic tests (up from 6)
  🐛 WiFi Apply Button - Now properly reboots device
  📚 Complete documentation updates

See RELEASE_NOTES.md for full details.

================================================================================
  FLASHING INSTRUCTIONS
================================================================================

REQUIREMENTS:
  - esptool.py installed (pip install esptool)
  - USB cable connected to ESP32-S3
  - Device in bootloader mode (hold BOOT, press RESET, release BOOT)

OPTION 1: Full Install (New Device or Clean Install)
-----------------------------------------------------

Windows:
  esptool.py --chip esp32s3 --port COM3 write_flash -z ^
    0x0 bootloader-esp32s3.bin ^
    0x8000 partitions-esp32s3.bin ^
    0x10000 firmware-esp32s3.bin ^
    0x410000 littlefs-esp32s3.bin

Linux/Mac:
  esptool.py --chip esp32s3 --port /dev/ttyUSB0 write_flash -z \
    0x0 bootloader-esp32s3.bin \
    0x8000 partitions-esp32s3.bin \
    0x10000 firmware-esp32s3.bin \
    0x410000 littlefs-esp32s3.bin

OPTION 2: Update Firmware Only (Keep Settings)
-----------------------------------------------

Windows:
  esptool.py --chip esp32s3 --port COM3 write_flash -z ^
    0x10000 firmware-esp32s3.bin

Linux/Mac:
  esptool.py --chip esp32s3 --port /dev/ttyUSB0 write_flash -z \
    0x10000 firmware-esp32s3.bin

OPTION 3: Update Web Interface Only
------------------------------------

Windows:
  esptool.py --chip esp32s3 --port COM3 write_flash -z ^
    0x410000 littlefs-esp32s3.bin

Linux/Mac:
  esptool.py --chip esp32s3 --port /dev/ttyUSB0 write_flash -z \
    0x410000 littlefs-esp32s3.bin

NOTES:
  - Replace COM3 (Windows) or /dev/ttyUSB0 (Linux/Mac) with your port
  - Use "Option 1" for first-time installations
  - Use "Option 2" to update firmware while keeping your settings
  - Use "Option 3" to update just the web interface

================================================================================
  POST-FLASH SETUP
================================================================================

1. After flashing, press RESET button or power cycle the device

2. Connect to WiFi access point:
   SSID: FPVGate_XXXX (where XXXX is unique ID)
   Password: fpvgate1

3. Open web interface:
   http://192.168.4.1

4. Configure your settings:
   - Go to Configuration → General
   - Set pilot name, frequency band, channel
   - Adjust RSSI thresholds (Enter RSSI, Exit RSSI)
   - Configure race settings (Min Lap Time, etc.)

5. (Optional) Configure Webhooks:
   - Go to Configuration → Webhooks
   - Enable Webhooks
   - Add IP addresses of external LED controllers
   - Go to Configuration → LED Setup → Gate LEDs
   - Enable Gate LEDs and select which events to trigger

6. (Optional) Connect to your WiFi:
   - Go to Configuration → WiFi & Connection
   - Enter your WiFi SSID and password
   - Click Apply (device will reboot)
   - Find device IP on your network
   - Access via http://YOUR_DEVICE_IP

================================================================================
  DESKTOP APP INSTALLATION
================================================================================

1. Extract FPVGate_Desktop_v1.4.0_Windows_x64.zip

2. Run FPVGate.exe

3. Select connection method:
   - USB Serial (COM port)
   - WiFi (IP address)

4. Use the desktop app to control your FPVGate device

================================================================================
  TROUBLESHOOTING
================================================================================

Flash Failed:
  - Ensure device is in bootloader mode
  - Check USB cable is data-capable (not charge-only)
  - Try a different USB port
  - Verify correct COM port with Device Manager (Windows) or ls /dev/tty* (Linux/Mac)

Can't Connect to WiFi:
  - Verify SSID: FPVGate_XXXX
  - Verify password: fpvgate1
  - Check device LED for status
  - Power cycle the device

Web Interface Not Loading:
  - Clear browser cache
  - Try http://192.168.4.1 (not https)
  - Check WiFi connection
  - Reflash littlefs-esp32s3.bin

Config Version Mismatch:
  - Config version bumped from 4 to 5 in v1.4.0
  - Settings will reset to defaults on first boot
  - This is expected - re-enter your settings

Webhooks Not Firing:
  - Ensure Webhooks are enabled (Configuration → Webhooks)
  - Ensure Gate LEDs are enabled (Configuration → LED Setup)
  - Ensure individual event toggles are enabled
  - Check webhook IP addresses are correct
  - Verify LED controller is on same network
  - Test with Test button to verify connectivity

================================================================================
  SUPPORT & DOCUMENTATION
================================================================================

GitHub Repository:
  https://github.com/LouisHitchcock/FPVGate

Documentation:
  https://github.com/LouisHitchcock/FPVGate/tree/main/docs

Report Issues:
  https://github.com/LouisHitchcock/FPVGate/issues

Discussions:
  https://github.com/LouisHitchcock/FPVGate/discussions

================================================================================
  LICENSE
================================================================================

Copyright (c) 2024 Louis Hitchcock

This project is licensed under the MIT License.
See LICENSE file in repository for details.

================================================================================
