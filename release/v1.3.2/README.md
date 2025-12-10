# FPVGate v1.3.2 - Installation Guide

## 📦 What's Included

### Firmware Binaries
- `firmware-esp32s3.bin` - Main firmware for ESP32-S3 DevKitC-1
- `firmware-esp32.bin` - Firmware for ESP32 (PhobosLT compatible)
- `firmware-licardotimer-s3.bin` - Firmware for LicardoTimer S3 variant

### Filesystem Images
- `littlefs-esp32s3.bin` - Web interface and data files for ESP32-S3
- `littlefs-esp32.bin` - Web interface and data files for ESP32
- `littlefs-licardotimer-s3.bin` - Web interface and data files for LicardoTimer S3

### Bootloaders & Partitions
- `bootloader-esp32s3.bin` - ESP32-S3 bootloader
- `partitions-esp32s3.bin` - ESP32-S3 partition table
- `bootloader-esp32.bin` - ESP32 bootloader
- `partitions-esp32.bin` - ESP32 partition table
- `bootloader-licardotimer-s3.bin` - LicardoTimer S3 bootloader
- `partitions-licardotimer-s3.bin` - LicardoTimer S3 partition table

### SD Card Audio
- `sd_card_contents.zip` - Voice packs for SD card (optional but recommended)
  - sounds_default
  - sounds_adam
  - sounds_antoni
  - sounds_rachel

## 🚀 Installation Methods

### Method 1: OTA Update (Easiest - No USB Required)

**Requirements:**
- Already running FPVGate v1.2.0 or later
- Connected to device's WiFi or web interface

**Steps:**
1. Open FPVGate web interface (http://192.168.0.177)
2. Go to **Configuration** tab
3. Scroll to **System Setup** section
4. Click **"Open Update Page"**
5. Click **"Choose File"** under Firmware
6. Select `firmware-esp32s3.bin` (or appropriate variant)
7. Click **"Update"**
8. Wait for upload and reboot (~30 seconds)
9. **Optional:** Upload `littlefs-esp32s3.bin` to update web interface

** Note:** After upgrade, config will be reset to defaults due to version change (v2 → v3).

---

### Method 2: USB Flashing with esptool.py

**Requirements:**
- Python 3.6+ installed
- esptool.py (`pip install esptool`)
- USB-C cable
- Device in download mode (hold BOOT button while connecting USB)

#### For ESP32-S3 DevKitC-1

**Full Flash (First Time Install):**
```bash
esptool.py --chip esp32s3 --port COM12 --baud 921600 \
  --before default_reset --after hard_reset write_flash -z \
  --flash_mode dio --flash_freq 80m --flash_size 8MB \
  0x0 bootloader-esp32s3.bin \
  0x8000 partitions-esp32s3.bin \
  0x10000 firmware-esp32s3.bin \
  0x410000 littlefs-esp32s3.bin
```

**Firmware Only Update:**
```bash
esptool.py --chip esp32s3 --port COM12 --baud 921600 \
  write_flash 0x10000 firmware-esp32s3.bin
```

**Filesystem Only Update:**
```bash
esptool.py --chip esp32s3 --port COM12 --baud 921600 \
  write_flash 0x410000 littlefs-esp32s3.bin
```

#### For ESP32 (PhobosLT)

**Full Flash:**
```bash
esptool.py --chip esp32 --port COM12 --baud 921600 \
  --before default_reset --after hard_reset write_flash -z \
  --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000 bootloader-esp32.bin \
  0x8000 partitions-esp32.bin \
  0x10000 firmware-esp32.bin \
  0x290000 littlefs-esp32.bin
```

**Replace `COM12` with your actual port:**
- Windows: `COM3`, `COM4`, etc.
- macOS: `/dev/cu.usbserial-*`
- Linux: `/dev/ttyUSB0`, `/dev/ttyACM0`

---

### Method 3: PlatformIO (For Developers)

**Requirements:**
- PlatformIO installed
- Source code cloned

**Build and Upload:**
```bash
# Build all environments
pio run

# Upload to specific port
pio run --target upload --upload-port COM12

# Upload filesystem
pio run --target uploadfs --upload-port COM12
```

**Build specific environment:**
```bash
pio run -e ESP32S3
pio run -e PhobosLT
pio run -e LicardoTimerS3
```

---

## 💾 SD Card Setup (Optional but Recommended)

### Preparing SD Card

1. **Format SD card:**
   - File system: FAT32
   - Size: 4GB-32GB recommended
   - Allocation size: Default (4096 bytes)

2. **Extract audio files:**
   - Unzip `sd_card_contents.zip`
   - Copy all `sounds_*` folders to SD card root
   - SD card structure should be:
     ```
     /sounds_default/
     /sounds_adam/
     /sounds_antoni/
     /sounds_rachel/
     ```

3. **Insert SD card:**
   - Power off device
   - Insert MicroSD card into slot
   - Power on device
   - Check Configuration → System Diagnostics → Storage test

### SD Card Benefits
- Voice announcements (ElevenLabs voices)
- Race history storage
- More storage than internal flash
- Easy to transfer data between devices

---

## 📝 After Installation

### First Boot Checklist

1. **Connect to WiFi:**
   - Device creates AP: `FPVGate_XXXX` (password: `fpvgate123`)
   - Or connect to your network in Configuration

2. **Reconfigure Settings:**
   - ⚠️ Config was reset due to version upgrade
   - Set Pilot Name, Callsign, Color
   - Set WiFi credentials (if using station mode)
   - Adjust RSSI thresholds (or use Calibration Wizard)

3. **Test Features:**
   - Run System Self-Test (Configuration → System Diagnostics)
   - Test audio announcements
   - Check LED effects
   - Verify SD card is recognized

4. **Run Calibration:**
   - Go to Calibration tab
   - Click "Start Calibration Wizard"
   - Fly 3 laps and mark peaks
   - Apply calculated thresholds

---

## 🔧 Troubleshooting

### Device Won't Boot
- **Check power supply:** 5V 2A minimum
- **Flash bootloader:** Use "Full Flash" command above
- **Erase flash first:** `esptool.py --chip esp32s3 --port COM12 erase_flash`
- **Hold BOOT button:** While connecting USB to enter download mode

### Upload Failed
- **Wrong port:** Check Device Manager (Windows) or `ls /dev/tty*` (Linux/Mac)
- **Wrong chip type:** Use `--chip esp32s3` or `--chip esp32` appropriately
- **Insufficient permissions:** Run terminal as administrator or use `sudo`
- **Driver issue:** Install CH340 or CP210x drivers

### OTA Upload Fails
- **File too large:** Firmware must be <2MB (current: ~1MB, you're good)
- **Connection timeout:** Move closer to device or use USB
- **Corrupted file:** Re-download binaries

### SD Card Not Detected
- **Wrong format:** Must be FAT32
- **Card too large:** >32GB may not work, use smaller card
- **Wiring issue:** Check SD card connections (see Hardware Guide)
- **Run self-test:** Configuration → System Diagnostics

### LEDs Not Working
- **Check wiring:** GPIO48 for ESP32-S3, GPIO21 for ESP32
- **Power issue:** LEDs need sufficient current (2A+ for full brightness)
- **Manual override:** Disable "Manual LED Control" to see race events

### No Audio
- **Check voice selection:** Configuration → TTS Settings
- **SD card missing:** ElevenLabs voices require SD card
- **Try PiperTTS:** Select "PiperTTS" from voice dropdown (no SD needed)
- **Volume:** Check device volume and speaker/headphone connection

---

## 📚 Documentation

- **User Guide:** [docs/USER_GUIDE.md](../../docs/USER_GUIDE.md)
- **Features:** [docs/FEATURES.md](../../docs/FEATURES.md)
- **Hardware Guide:** [docs/HARDWARE_GUIDE.md](../../docs/HARDWARE_GUIDE.md)
- **Getting Started:** [docs/GETTING_STARTED.md](../../docs/GETTING_STARTED.md)
- **Development:** [docs/DEVELOPMENT.md](../../docs/DEVELOPMENT.md)

## 🐛 Reporting Issues

Found a bug? Please report it on GitHub:
https://github.com/LouisHitchcock/FPVGate/issues

Include:
- FPVGate version (v1.3.2)
- Hardware (ESP32-S3, ESP32, etc.)
- Steps to reproduce
- Serial output if possible
- Expected vs actual behavior

## 📄 License

FPVGate is released under the GNU General Public License v3.0.
See [LICENSE](../../LICENSE) file for details.

## 🙏 Acknowledgments

Based on PhobosLT by phobos-
Enhanced and maintained by Louis Hitchcock

---

**Happy Racing! 🏁**
