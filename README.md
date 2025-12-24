# FPVGate

**Personal FPV Lap Timer for ESP32-C6 Hardware coming soon!**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32--S3-orange.svg)](https://platformio.org/)
[![GitHub release](https://img.shields.io/github/v/release/LouisHitchcock/FPVGate)](https://github.com/LouisHitchcock/FPVGate/releases)

A compact, self-contained RSSI-based lap timing solution for 5.8GHz FPV drones. Perfect for personal practice sessions, small indoor tracks, and training.

![FPVGate](assets/logo-black.svg)

---

## What is FPVGate?

FPVGate measures lap times by detecting your drone's video transmitter signal strength (RSSI). When you fly through the gate, it detects the peak signal and records your lap time. No transponders, no complex setup—just plug in, calibrate, and fly.

### Screenshots

| Race Screen | Configuration Menu |
|:-----------:|:------------------:|
| ![Race Screen](screenshots/12-12-2025/Race%20-%2012-12-2025.png) | ![Config Menu](screenshots/12-12-2025/Config%20Screen%20-%20Pilot%20Info%2012-12-2025.png) |

| Calibration Wizard | Race History |
|:------------------:|:------------:|
| ![Calibration Wizard](screenshots/12-12-2025/Calibration%20Wizard%20Recording%20-%2012-12-2025.png) | ![Race History](screenshots/12-12-2025/Race%20History%20-%2012-12-2025.png) |

### Key Features

**Dual Connectivity**
- WiFi Access Point (works with any device)
- Configured to allow Wifi connection, and simultaneously allow your device to connect to cellular internet 
- USB Serial CDC (zero-latency local connection)
- Electron desktop app for Windows/Mac/Linux

**Visual Feedback**
- Real-time WiFi status display with connection monitoring
- Real-time RSSI visualization
- OSD overlay for live streaming (transparent, multi-monitor support)
- Mobile-responsive web interface

**Natural Voice Announcements**
- Pre-recorded ElevenLabs voices (4 voices included)
- PiperTTS for low-latency synthesis
- Phonetic name support
- Configurable announcement formats

**Race Analysis**
- Real-time lap tracking with gap analysis
- Fastest lap highlighting
- Fastest 3 consecutive laps (RaceGOW format)
- Save your race history to your client device and re-import later
- Marshalling mode for post-race lap editing (add/remove/edit laps)
- Detailed race analysis view
- Config backup/restore

**Webhooks & Integration**
- HTTP webhook support for external LED controllers
- Configurable triggers (race start/stop, laps)
- Gate LED control with granular event settings
- Network-based device integration

**Developer Friendly**
- Comprehensive self-test diagnostics (19 tests)
- OTA firmware updates
- Transport abstraction layer
- Open source (MIT License)

**Supported Bands:**
- **A (Boscam A)** - 5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725
- **B (Boscam B)** - 5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866
- **E (Boscam E)** - 5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945
- **F (Fatshark)** - 5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880
- **R (RaceBand)** - 5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917
- **L (LowBand)** -  5362, 5399, 5436, 5473, 5510, 5547, 5584, 5621
---

## Quick Start

### 1. Hardware You'll Need

Pre-made and flashed hardware link coming soon!

**[Detailed setup guide →](docs/GETTING_STARTED.md)**

### 2. Connect and Configure

**WiFi (Default):**
1. Connect to `FPVGate_XXXX` network (password: `fpvgate1`)
2. Open `http://192.168.4.1` in your browser. 
3. Go to Configuration → Set your VTx frequency
4. Go to Calibration → Set RSSI thresholds
5. Start racing!

**USB (Electron App - and Android and Windows apps in development):**
1. Download [Electron app from releases](https://github.com/LouisHitchcock/FPVGate/releases)
2. Connect via USB
3. Launch app and select COM port
4. All features work identically to WiFi mode

**[Complete user guide →](docs/USER_GUIDE.md)**
---

## Documentation

**[Getting Started](docs/GETTING_STARTED.md)** - Hardware setup, wiring, flashing, initial config  
**[User Guide](docs/USER_GUIDE.md)** - How to connect, calibrate, race, and use features  
**[Features Guide](docs/FEATURES.md)** - In-depth feature documentation   

**Additional Docs:**
- [Quick Start](QUICKSTART.md) - Fast track for experienced users

---

## How It Works

FPVGate uses an RX5808 video receiver module to monitor your drone's RSSI (signal strength). As you fly through the gate:

1. **Approach** - RSSI rises above Enter threshold → crossing starts
2. **Peak** - RSSI peaks when you're closest to the gate
3. **Exit** - RSSI falls below Exit threshold → lap recorded

The time between peaks = your lap time. Simple, reliable, and accurate.

```
RSSI  │     /\
      │    /  \
      │   /    \     ← Single clean peak
Enter ├──/──────\───
      │ /        \
Exit  ├/──────────\─
      └─────────────── Time
```

---

## Project Status

**Current Version:** v1.0.0 - forked and ported from LouisHitchcock FPVGate v1.4.0  
**Platform:** Seeduino XIAO ESP32-C6
**License:** MIT  
**Status:** Stable Beta - actively maintained

### Recent Updates

**v0.9.0 (Latest - Beta)**
- Forked and ported to Seediuno ESP32-C6 for specific hardware available for purchase.
- Removed Track Management and auto race saving because we don't use an SD Card.
- Added the ability to export/import the current race in memory.
- Cleanly hides the SDCard Tracks, VBat and LED functionality if those pins aren't defined.
- Removed captive DNS so the mobile device simultaneously connects to the cellular network at the same time.
- Fixed a bug where we were stuck on RaceBand Ch 1.
- 

**v1.4.0 (forked from LouisHitchcock)**
- Track Management System - Create, edit, and manage track profiles with images
- Distance Tracking - Real-time distance display and statistics
- Webhook System - HTTP webhooks for external LED controller integration
- Gate LED Control - Granular control over webhook triggers (race start/stop, laps)
- Enhanced Self-Tests - Comprehensive diagnostics for all device features (19 tests)
- WiFi Reboot Fix - Apply WiFi settings button now properly reboots device
- Enhanced Race Editing - Edit race metadata, lap times, and track associations
- Track Selection - Choose active track before racing (persists to EEPROM)
- Improved Configuration UI - Full-screen modal with organized sections

**v1.3.3**
- Modern Configuration UI with full-screen overlay modal
- WebSocket Stability with SSE keepalive mechanism
- Fixed duplicate element IDs and WiFi configuration issues

**v1.3.2**
- WiFi Status Display with real-time connection monitoring
- Marshalling Mode for post-race lap editing
- LED settings persistence to EEPROM

**v1.3.1**
- Fixed race history storage initialization bug
- Improved calibration wizard with 3-peak marking system
- Enhanced threshold calculation (peak-relative instead of baseline-relative)
- Added visual smoothing to calibration chart for easier peak identification

**v1.3.0**
- iOS/Safari full audio support with vibration feedback
- Mobile-responsive interface for phones and tablets
- USB Serial CDC connectivity with Electron desktop app
- OSD overlay system for streaming (transparent, real-time)
- Cross-device race storage on SD card
- Race tagging, naming, and detailed analysis
- Enhanced race management with import/export

**v1.2.1**
- SD card storage for audio files
- Automatic flash-to-SD migration
- Enhanced OTA capacity (2MB updates)
- Graceful fallback to LittleFS

**v1.2.0**
- System self-test diagnostics
- PiperTTS integration
- Enhanced LED presets with color pickers
- Complete config export/import

**[Full changelog →](CHANGELOG.md)**

---

## Community & Support

- **Bug Reports:** [GitHub Issues](https://github.com/LouisHitchcock/FPVGate/issues)
- **Feature Requests:** [GitHub Discussions](https://github.com/LouisHitchcock/FPVGate/discussions)
- **Star this repo** if you find it useful!

---

## Credits

FPVGate is a heavily modified fork of [PhobosLT](https://github.com/phobos-/PhobosLT) by phobos-. The original project provided the foundation for RSSI-based lap timing.

Portions of the timing logic are inspired by [RotorHazard](https://github.com/RotorHazard/RotorHazard).

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

**Made with care for the FPV community**
