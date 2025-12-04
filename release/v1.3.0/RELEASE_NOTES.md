# FPVGate v1.3.0 Release Notes

**Release Date:** December 4, 2025

## What's New

### Mobile & iOS Support
- **iOS/Safari Audio Support**: Full audio functionality on iOS devices with automatic audio context unlocking
- **Mobile-Responsive Interface**: Optimized web interface for phones and tablets with adaptive layout
- **Vibration Feedback**: Race start vibration works even when phone is in silent mode
- **Touch-Optimized Controls**: Mobile-friendly buttons, navigation, and configuration options

### Cross-Device Race Storage
- **SD Card Race Storage**: Races now stored on SD card for access across all connected devices
- **Shared Race History**: View and manage races from any device connected to FPVGate
- **Automatic Directory Creation**: Creates `/sd/races/` directory on initialization
- **LittleFS Fallback**: Automatically uses internal storage if SD card unavailable

### Documentation
- **Comprehensive Wiki**: New wiki-style documentation with dedicated guides
  - Getting Started Guide
  - User Guide
  - Hardware Guide
  - Features Documentation
  - Development Guide
- **Updated README**: Clearer project overview and setup instructions

### USB Connectivity & Desktop Integration
- **USB Serial CDC**: Direct USB connection for low-latency communication
- **Electron Desktop App**: Native desktop application for Windows, macOS, and Linux
- **USB Transport Layer**: WebUSB and native USB support for browser and desktop
- **Unified API**: Single codebase works over both WiFi and USB connections
- **Auto-Detection**: Automatically detects and switches between USB and WiFi modes

### OSD Overlay System
- **Live Race Display**: Dedicated OSD page for race timing display
- **Real-Time Updates**: Server-sent events for instant lap time updates
- **Transparent Overlay**: Can be overlaid on video feeds using OBS/StreamLabs
- **Customizable Layout**: Show pilot name, current lap, lap times, and race timer
- **Multi-Monitor Support**: Open OSD in separate window for streaming setups
- **URL Sharing**: One-click copy OSD URL for easy setup

## Improvements

### Race Management
- **Race History Export/Import**: Download and import race data as JSON
- **Individual Race Export**: Download single races for sharing
- **Race Tagging**: Add custom tags to races for organization
- **Race Naming**: Name races for easy identification
- **Race Details View**: Comprehensive analysis of each race with lap-by-lap breakdown
- **Fastest Round Analysis**: Identify best consecutive 3-lap combinations
- **Race Search & Filter**: Find races by name, tag, or date

### Audio System
- **Shared AudioContext**: Beep tones use persistent AudioContext to prevent iOS suspension
- **Audio Unlock on User Interaction**: Unlocks audio during "Enable Audio" button press
- **PiperTTS iOS Compatibility**: Web Audio API properly resumed on iOS/Safari
- **Web Speech API Loading**: Ensures voices are loaded on iOS before use
- **Multi-Voice Support**: Switch between default, Rachel, Adam, and Antoni voices
- **Voice Preview**: Test voices before selecting

### User Interface
- **Responsive Tables**: Race history and lap tables adapt to screen size
- **Flexible Navigation**: Navigation links wrap on small screens
- **Mobile Charts**: Optimized chart heights for mobile viewing
- **Single-Column Stats**: Statistics boxes stack vertically on phones
- **Theme Support**: Multiple color themes (Oceanic, Darker, Lighter, Midnight)
- **LED Control**: Configure RGB LED patterns, colors, speed, and brightness
- **Battery Monitoring**: Track battery voltage with configurable alerts

## Bug Fixes
- Fixed PiperTTS not working on Safari/iOS due to suspended AudioContext
- Fixed race start beep tones not playing on mobile devices
- Fixed USBSerial compilation error in selftest module
- Fixed audio playback requiring multiple user interactions on iOS

## Technical Changes
- **Race Storage**: Moved from `/races.json` to `/sd/races/races.json` for SD card storage
- **USB CDC Integration**: Added USB Serial CDC support for native desktop connectivity
- **WebUSB Support**: Browser-based USB communication without drivers
- **Server-Sent Events**: Real-time race updates for OSD and multi-client sync
- **Transport Abstraction**: Unified API layer supporting WiFi, WebUSB, and native USB
- **Electron App**: Desktop application built with Electron + serialport
- **iOS Audio Unlock**: Safari/iOS audio context unlocking on user interaction
- **Vibration API**: Mobile haptic feedback for race start
- **Responsive CSS**: Media queries for mobile, tablet, and desktop layouts
- **OSD HTML/CSS/JS**: Dedicated overlay page with transparent background

## Installation

### Full Flash (Recommended for new installations)
Use `esptool.py` or ESP Flash Tool to flash:
1. `bootloader.bin` at `0x1000`
2. `partitions.bin` at `0x8000`
3. `FPVGate_v1.3.0_firmware.bin` at `0x10000`
4. `FPVGate_v1.3.0_filesystem.bin` at `0x410000`

### OTA Update (From v1.2.x)
1. Navigate to Settings > OTA Update in the web interface
2. Upload `FPVGate_v1.3.0_firmware.bin`
3. After firmware update, upload `FPVGate_v1.3.0_filesystem.bin` via LittleFS upload

### SD Card Setup
Copy the contents of `sounds_TEMP/` folder to your SD card root to enable pre-recorded voice announcements:
- `sounds_default/` - Default voice pack
- `sounds_rachel/` - Rachel voice (ElevenLabs)
- `sounds_adam/` - Adam voice (ElevenLabs)
- `sounds_antoni/` - Antoni voice (ElevenLabs)

### Desktop App Installation
The Electron desktop app is available in the `electron/` directory:
1. Install Node.js (v16 or later)
2. Navigate to `electron/` folder
3. Run `npm install`
4. Run `npm start` to launch the app
5. Connect FPVGate via USB and select the COM port

## Compatibility
- **Hardware**: ESP32-S3 DevKitC-1 (8MB Flash, No PSRAM)
- **Browsers**: Chrome, Firefox, Safari (iOS 12+), Edge
- **WebUSB**: Chrome 89+, Edge 89+ (Windows, macOS, Linux)
- **Desktop App**: Windows 10+, macOS 10.13+, Linux (Ubuntu 18.04+)
- **Mobile**: iOS 12+, Android 8+
- **SD Card**: FAT32 formatted, 4GB-32GB recommended
- **Streaming Software**: OBS Studio, StreamLabs OBS, XSplit

## Known Issues
- iOS silent mode still mutes audio (by design, cannot be bypassed by web apps)
- Vibration API only works on mobile devices (not supported on desktop)
- WebUSB not available on Firefox or Safari (Chrome/Edge only)
- OSD overlay transparency may require green screen in some streaming setups
- Race migration from LittleFS to SD card not automatic

## Upgrade Notes
- **Race Data**: Races stored in LittleFS will not automatically migrate to SD card
  - Download race history via web interface before upgrading
  - Import races after upgrade to restore them to SD card storage
- **Voice Files**: Re-copy voice packs to SD card after filesystem update
- **USB Connectivity**: Enable USB CDC in Arduino IDE if rebuilding firmware
- **Desktop App**: Install Node.js dependencies before first run

## Credits
Developed by the FPVGate team with community contributions.

---

For full documentation, visit: https://github.com/LouisHitchcock/FPVGate
