# Changelog

All notable changes to FPVGate will be documented in this file.

## [1.3.2] - 2024-12-10

### Added
- **LED Settings Persistence** - All LED configuration now saves to EEPROM and persists across reboots
  - Added `ledPreset`, `ledSpeed`, `ledFadeColor`, `ledStrobeColor`, `ledManualOverride` to config struct
  - Config version bumped to v3 for automatic migration
  - LED settings automatically restored on device boot
  - Page refresh now loads current LED state from device
  - All LED changes (/led/preset, /led/brightness, /led/speed, /led/color, /led/fadecolor, /led/strobecolor, /led/override) now save to config

### Changed
- **Default LED Preset** - Rainbow Wave is now the default LED effect (was Solid Colour in web UI)
- **Config JSON Buffer** - Increased from 256 to 512 bytes to accommodate expanded LED settings
- **Frontend LED Loading** - Web interface now properly loads all LED settings from device config on page load
  - Loads preset, brightness, speed, all colors, and manual override state
  - Removed old ledMode (0-3) mapping logic in favor of direct ledPreset usage

### Fixed
- **LED Settings Reset on Page Refresh** - LED configuration now persists properly instead of reverting to defaults
- **Solid Colour Default Bug** - Fixed page loading with wrong LED preset (was showing Solid Colour instead of saved preset)

### Technical
- Updated `lib/CONFIG/config.h` - Added 5 new LED config fields, incremented CONFIG_VERSION to 3
- Updated `lib/CONFIG/config.cpp` - Added getters/setters, JSON serialization, and defaults for new LED fields
- Updated `lib/WEBSERVER/webserver.cpp` - All LED endpoints now call config setters to persist changes
- Updated `src/main.cpp` - LED initialization now loads all settings from config (preset, speed, colors, override)
- Updated `data/script.js` - Page load reads LED config and populates all UI elements accordingly
- Config struct size: ~140 bytes (well within 256 byte EEPROM reservation)

## [1.3.1] - 2024-12-08

### Fixed
- **Race History Storage** - Fixed race history not being initialized with storage backend
  - Races now properly save to SD card/LittleFS after each session
  - Added automatic race history reload when SD card is mounted
  - Previously, race history was not persisting across reboots
- **Calibration Wizard Threshold Calculation** - Complete overhaul for better accuracy
  - Now calculates thresholds as drops from peak (25% and 40%) instead of rises from baseline
  - Entry RSSI: 25% down from peak (catches rising edge well into spike)
  - Exit RSSI: 40% down from peak (catches falling edge, still above baseline)
  - Typically results in ~20-30 RSSI difference (was often 60+ before)
  - Much more accurate and intuitive threshold values

### Changed
- **Calibration Wizard UI** - Simplified to 3-peak marking system
  - Users now only mark the 3 highest peaks (one per lap)
  - Previously required 6 marks (entry and exit for each lap)
  - Updated instructions to clarify peak-only marking
- **Calibration Chart Display** - Enhanced visual smoothing
  - Added 15-point moving average filter (visual only, doesn't affect data)
  - Added filled area under RSSI line matching main chart style
  - Makes peaks much easier to identify and mark accurately

### Technical
- Updated `src/main.cpp` - Added `raceHistory.init(&storage)` call
- Updated `data/index.html` - Simplified wizard instructions
- Updated `data/script.js` - New threshold calculation and visual smoothing
- Cleaned up temporary files and test directories from repository

## [1.3.0] - 2024-12-04

### Added - Mobile & iOS Support
- **iOS/Safari Audio Support**: Full audio functionality on iOS devices
  - Automatic audio context unlocking on user interaction
  - Shared AudioContext for beep tones to prevent suspension
  - Web Audio API properly resumed on iOS/Safari
  - Web Speech API voice loading for iOS
- **Mobile-Responsive Interface**: Optimized web interface for phones and tablets
  - Responsive tables and navigation with flex-wrap
  - Mobile-optimized chart heights (250px)
  - Single-column stat boxes on phones
  - Touch-friendly buttons and controls
- **Vibration Feedback**: Race start vibration (works even in silent mode)
  - 500ms vibration on race start
  - Vibration API integration for mobile devices

### Added - USB Connectivity & Desktop
- **USB Serial CDC**: Direct USB connection for low-latency communication
  - Zero-latency local connection
  - Automatic USB/WiFi detection and switching
- **Electron Desktop App**: Native desktop application
  - Windows, macOS, and Linux support
  - Native USB connectivity via serialport
  - Full feature parity with web interface
  - Bundled with all dependencies
- **Transport Abstraction Layer**: Unified API for WiFi and USB
  - `usb-transport.js` - WebUSB and native USB support
  - Single codebase works over both WiFi and USB
  - Automatic transport selection

### Added - OSD Overlay System
- **Live Race Display**: Dedicated OSD page for streaming
  - Real-time lap time updates via Server-Sent Events
  - Transparent background for OBS/StreamLabs overlay
  - Customizable layout with pilot name, lap times, race timer
  - Multi-monitor support
  - One-click URL copying for easy setup
- **OSD HTML/CSS/JS**: Complete overlay implementation
  - `osd.html`, `osd.css`, `osd.js`
  - Event-driven architecture for real-time updates

### Added - Cross-Device Race Storage
- **SD Card Race Storage**: Races stored on SD card for shared access
  - Race storage path: `/races.json` → `/sd/races/races.json`
  - Automatic `/sd/races/` directory creation
  - Cross-device accessibility (all connected devices see same data)
  - LittleFS fallback if SD card unavailable

### Added - Enhanced Race Management
- **Race Tagging**: Add custom tags to races for organization
- **Race Naming**: Name races for easy identification
- **Race Details View**: Comprehensive analysis with lap-by-lap breakdown
- **Fastest Round Analysis**: Best consecutive 3-lap combinations
- **Individual Race Export**: Download single races for sharing
- **Race Search & Filter**: Find races by name, tag, or date

### Added - Documentation
- **Comprehensive Wiki**: New wiki-style documentation
  - `docs/GETTING_STARTED.md` - Setup and flashing guide
  - `docs/USER_GUIDE.md` - Complete feature walkthrough
  - `docs/HARDWARE_GUIDE.md` - Components and wiring
  - `docs/FEATURES.md` - In-depth technical documentation
  - `docs/DEVELOPMENT.md` - Building and contributing
- **Updated README**: Clearer project overview and quick start

### Changed
- **Audio System**: iOS/Safari compatibility improvements
  - Beep tones use persistent AudioContext
  - Audio unlock during "Enable Audio" button press
  - PiperTTS AudioContext resume on iOS
- **LED Control**: Enhanced configuration options
  - Pattern selection with color pickers
  - Speed and brightness controls
  - Manual override mode
- **Battery Monitoring**: Improved UI with voltage tracking and alerts
- **Theme Support**: Multiple color themes (Oceanic, Darker, Lighter, Midnight)

### Fixed
- **PiperTTS on iOS**: Fixed suspended AudioContext preventing audio playback
- **Race Start Beeps on Mobile**: Fixed beep tones not playing on iOS/Safari
- **USBSerial Compilation Error**: Changed `USBSerial` to `Serial` for ESP32-S3
- **Audio Requiring Multiple Interactions**: Single interaction now unlocks all audio

### Technical
- **Server-Sent Events**: Real-time race updates for OSD and multi-client sync
- **WebUSB Support**: Browser-based USB communication
- **Electron Integration**: Desktop app with serialport
- **iOS Detection**: Platform and user agent detection for iOS/Safari
- **Media Queries**: Responsive CSS for mobile, tablet, desktop
- **Race Storage Migration**: Moved from internal flash to SD card

### Compatibility
- **Hardware**: ESP32-S3 DevKitC-1 (8MB Flash)
- **Browsers**: Chrome, Firefox, Safari (iOS 12+), Edge
- **WebUSB**: Chrome 89+, Edge 89+
- **Desktop App**: Windows 10+, macOS 10.13+, Linux (Ubuntu 18.04+)
- **Mobile**: iOS 12+, Android 8+
- **SD Card**: FAT32, 4GB-32GB recommended

### Known Issues
- iOS silent mode mutes audio (by design, cannot be bypassed)
- Vibration API only works on mobile devices
- WebUSB not available on Firefox or Safari
- Race migration from LittleFS to SD card not automatic

## [1.2.1] - 2024-12-01

### Added
- **SD Card Storage Support** - Audio files now stored on MicroSD card
- SD card pin configuration: GPIO39 (CS), GPIO36 (SCK), GPIO35 (MOSI), GPIO37 (MISO)
- Automatic sound migration from LittleFS to SD card on first boot
- `Storage::migrateSoundsToSD()` - Migration utility method
- `Storage::isSDAvailable()` - SD card status check
- SD-aware audio file streaming with LittleFS fallback
- Self-test diagnostics for SD card functionality
- SD_CARD_MIGRATION_GUIDE.md documentation

### Changed
- **Partition table optimized** - LittleFS reduced from 4.4MB to 1MB
- **OTA partition size increased** - Each slot now 2MB (was 1.8MB)
- Audio files served from SD card first, LittleFS fallback
- README.md updated with SD card wiring and setup
- QUICKSTART.md updated with SD card instructions
- WARP.md updated with SD card architecture notes

### Fixed
- Flash storage constraints - 85% more free space available
- OTA update size limitations - Now supports 2MB firmware

### Technical Details
- Deferred SD initialization (5 seconds after boot) prevents watchdog timeout
- Non-blocking audio streaming directly from SD card
- Graceful degradation if SD card unavailable or fails

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.2.0] - 2025-12-01

### Added
- **System Self-Test Functionality**: Comprehensive diagnostics for hardware and software validation
  - Tests: RX5808 RSSI module, Lap Timer, Audio/Buzzer, Configuration, Race History, Web Server, OTA Updates, Storage (LittleFS), EEPROM, WiFi, Battery Monitor, RGB LED
  - Accessible via "System Diagnostics" section in Configuration page
  - Visual pass/fail indicators with test duration and detailed error messages
  - REST API endpoint `/api/selftest` for programmatic access
- **PiperTTS Integration**: Lower latency text-to-speech option
  - **Exclusive Mode**: When PiperTTS is selected, it's used exclusively for all announcements (no pre-recorded file attempts)
  - **Fallback Mode**: ElevenLabs voices now fall back to PiperTTS only when pre-recorded files fail
  - Added to Voice dropdown as "PiperTTS (Lower Latency)"
  - Automatic TTS engine selection based on voice choice
- **Enhanced LED Presets**:
  - Renamed "Custom Color" to "Solid Colour" and moved to position 1
  - **Color Pickers**: Added for Solid Colour, Color Fade, and Strobe presets
  - **New Preset**: "Pilot Colour" - uses pilot's color from Pilot Info section
  - **Slowed Effects**: Police (20x slower) and Strobe (15x slower), scaled by effect speed
  - Removed redundant presets: Red Pulse, Green Solid, Blue Pulse, White Solid
- **Enhanced Config Import/Export**: Now includes all frontend settings
  - Theme, Audio settings (format, voice, TTS engine)
  - Pilot settings (callsign, phonetic, color)
  - LED settings (preset, colors, speed, manual override)
  - Battery monitoring toggle state

### Changed
- **UI Organization**: Created dedicated "TTS Settings" section in Configuration page
  - Groups: Announcer Type, Lap Announcement Format, Voice, Announcer Rate, Voice Control buttons
  - Improved logical organization of audio-related settings
- **Voice Selection Logic**: Automatically sets TTS engine based on selected voice
  - PiperTTS voice → sets engine to 'piper'
  - ElevenLabs voices → sets engine to 'webspeech'
- **LED Preset Organization**: Reordered for better usability
  - Order: Off, Solid Colour, Rainbow, Color Fade, Fire, Ocean, Police, Strobe, Comet, Pilot Colour
- **Battery Monitoring**: Now unchecked by default

### Fixed
- **Audio Infinite Loop Bug**: Fixed recursive `speak()` calls when pilot-specific audio files weren't found
  - Changed to direct `useTtsFallback()` calls in `speakComplexWithPilot()` and `speakComplexLapTime()`
  - Prevents infinite recursion when custom pilot audio is missing
- **Voice Control Button Styling**: Removed opacity from disabled state for better visibility
- **Toggle Switch Stretching**: Fixed CSS selector to exclude toggle switches from min-width rule
- **Include Guards**: Added proper include guards to RX5808.h, kalman.h, and laptimer.h
  - Prevents redefinition errors during compilation
  - Enables modular header inclusion

### Removed
- **TTS Fallback Engine Dropdown**: Removed from UI (now automatically determined by voice selection)

### Technical
- **Memory Usage**: Flash: 54.3% (996,581 bytes), RAM: 16.7% (54,632 bytes)
- **New Test Infrastructure**: SelfTest class with 13 individual test methods
- **Forward Declarations**: Used in selftest.h to avoid circular dependencies
- **Filesystem**: 4,653,056 bytes (1,792,357 compressed) - includes updated UI and audio files

## [1.1.0] - 2025-11-28

### Added - Natural Voice TTS System
- **Hybrid TTS System** with three-tier fallback:
  - Primary: ElevenLabs pre-recorded audio (natural, high-quality)
  - Secondary: Piper TTS via WASM (good quality, offline)
  - Fallback: Web Speech API (browser default)
- **Natural Number Pronunciation**: Numbers 0-99 spoken naturally
  - "11.44" → "eleven point forty-four" (not "one one point four four")
  - Instant audio transitions with <50ms gaps between clips
- **Voice Generation Scripts**:
  - `generate_voice_files.py` - Main generator with ElevenLabs API
  - `generate_all_voices.py` - Generate all 4 voice options
  - Support for 4 voices: Sarah (energetic), Rachel (calm), Adam (male), Antoni (male)
- **Comprehensive Audio Library**:
  - 100 number files (0-99) for natural time announcements
  - Race control phrases (arm_your_quad, starting_tone, gate_1, race_complete)
  - Lap numbers 1-50 for "Lap X" announcements
  - Pilot-specific audio (pilot_lap.mp3, test_sound.mp3)
- **Optimized Audio Playback**:
  - 1.3x playback speed for faster announcements
  - Audio caching for instant replay
  - `preservesPitch = false` for better quality at higher speeds
  - Early audio completion detection (50ms before end)

### Added - UI/UX Improvements
- **Redesigned Lap Table**:
  - New columns: Lap No | Lap Time | Gap | Total Time
  - Gap column shows difference from previous lap (+0.23s or -0.15s)
  - Total Time shows cumulative race time
  - Data attributes for lap indexing
- **Fastest Lap Highlighting**:
  - Gold/orange highlight (#f39c12) on fastest lap row
  - Subtle glow effect for visual distinction
  - Dynamically updates as new laps are completed
  - Excludes Gate 1 from fastest lap calculation
- **Enhanced Lap Analysis Stats**:
  - **Fastest Lap**: Shows lap number (e.g., "Lap 5" or "Gate 1")
  - **Fastest 3 Consecutive**: NEW - RaceGOW format support (e.g., "G1-L1-L2")
  - **Best 3 Laps**: Sum of 3 fastest individual laps (non-consecutive)
  - **Median Lap**: Statistical middle lap time
- **Configurable Lap Announcement Formats**:
  - Full: "Louis Lap 5, 12.34"
  - Lap + Time: "Lap 5, 12.34"
  - Time Only: "12.34"
  - Saved to localStorage for persistence

### Changed
- **"Hole Shot" → "Gate 1"**: More intuitive terminology
- **Race Start Audio**: Fixed timing - beep now plays AFTER voice announcements complete
- **Stop Race**: Now clears audio queue to prevent race start sounds
- **Audio File Organization**: Custom 3.94MB filesystem partition for expanded storage
- **Partition Table**: Custom 8MB layout (2MB app0 + 2MB app1 + 3.94MB filesystem)

### Fixed
- **Race stop beep bug**: Removed unwanted start beep when stopping race
- **Audio gaps**: Eliminated 200ms pauses between audio clips
- **Queue processing**: Removed 100ms gaps between announcements
- **Fastest lap color**: Changed from primary theme color to gold for distinction
- **Lap time gaps**: Now calculated correctly (difference from previous lap, not total)

### Technical
- **Audio Caching**: Map-based caching system for instant audio replay
- **Smart Preloading**: `oncanplay` event for faster first-play
- **Filesystem**: Upgraded from 1.5MB to 3.94MB partition
- **Audio Files**: 217 total files, 3.1MB (2.3MB compressed on device)
- **API Usage**: ~5,000 characters of ElevenLabs API (50% of free tier)

### Documentation
- Added `VOICE_GENERATION_README.md` - Complete voice setup guide
- Added `CHANGELOG.md` - Version history tracking
- Updated README.md with v1.1.0 features

### Developer Experience
- Python scripts for voice generation with progress tracking
- Auto-save configuration for voice selection and lap format
- localStorage persistence for frontend-only settings
- Comprehensive console logging for audio debugging

## [1.0.0] - 2025-11-XX

### Initial Release
- Single-node RSSI-based lap timing
- ESP32-S3 support with RX5808 module
- RGB LED indicators (NeoPixel support)
- Web interface with 23 themes
- Basic voice announcements (Web Speech API)
- Real-time RSSI calibration graph
- Race history with lap analysis
- WiFi Access Point mode
- OTA firmware updates
- Battery monitoring with alarms
- Configurable lap count (finite/infinite)
- Pilot profiles with callsigns
- Manual lap entry
- Config import/export

---

## Version Numbering

FPVGate follows [Semantic Versioning](https://semver.org/):
- **MAJOR**: Breaking changes or complete rewrites
- **MINOR**: New features, backwards compatible
- **PATCH**: Bug fixes, minor improvements

[1.2.0]: https://github.com/LouisHitchcock/FPVGate/releases/tag/v1.2.0
[1.1.0]: https://github.com/LouisHitchcock/FPVGate/releases/tag/v1.1.0
[1.0.0]: https://github.com/LouisHitchcock/FPVGate/releases/tag/v1.0.0
