FPVGate Desktop App v1.3.3 - Windows x64
==========================================

WHAT'S INCLUDED
---------------
FPVGate_Desktop_v1.3.3_Windows_x64.zip (103 MB)
- Portable Windows desktop application
- No installation required
- Full USB and WiFi connectivity


INSTALLATION
------------
1. Extract the ZIP file to any folder (e.g., C:\FPVGate)
2. Run FPVGate.exe
3. That's it! No installation needed.


QUICK START
-----------
USB Mode (Recommended):
1. Connect your FPVGate device via USB
2. Launch FPVGate.exe
3. The app will auto-detect your device
4. If multiple devices found, select from dropdown in Settings

WiFi Mode:
1. Connect to your FPVGate WiFi network (FPVGate_XXXX)
2. Launch FPVGate.exe
3. App will connect automatically


NEW FEATURES IN v1.3.3
----------------------
 Modern Configuration UI - Full-screen modal with 6 organized sections
 Application Menu - Native menu bar with keyboard shortcuts
 OSD Window - Transparent overlay for streaming (Ctrl+O)
 Marshalling Mode - Edit saved races (add/remove laps)
 Enhanced Calibration - Simplified 3-peak marking wizard
 LED Persistence - LED settings saved to EEPROM
 WiFi Status Display - Real-time connection monitoring (WiFi mode)


KEYBOARD SHORTCUTS
------------------
Ctrl+O    - Open OSD overlay window
Ctrl+R    - Refresh connection
F11       - Toggle fullscreen
F12       - Toggle DevTools (for debugging)
Alt+F4    - Exit application


FEATURES
--------
Core:
- USB Serial CDC connection (~10ms latency)
- WiFi connection (~50-100ms latency)
- Auto-detection of ESP32 devices
- Dual mode switching

Racing:
- Real-time lap timing with gap analysis
- Fastest lap highlighting
- Fastest 3 consecutive laps (RaceGOW format)
- Race history with tags and names
- Marshalling mode for post-race editing

Configuration:
- 6-section modern UI (Lap/Pilot/LED/WiFi/System/Diagnostics)
- 10 LED presets with persistence
- Multi-voice TTS (4 ElevenLabs + PiperTTS)
- Theme selector (23 themes)
- Self-test diagnostics

Calibration:
- Interactive 3-peak marking wizard
- Automatic threshold calculation
- Real-time RSSI visualization


USB MODE LIMITATIONS
--------------------
These features require WiFi and won't work in USB mode:
 WiFi Status Display (API not accessible)
 Firmware OTA Updates (requires WiFi)
 SSE Keepalive (not applicable to USB)

All other features work identically in both modes!


SYSTEM REQUIREMENTS
-------------------
- Windows 10/11 (64-bit)
- 500 MB free disk space
- USB 2.0 port (for USB mode)
- WiFi adapter (for WiFi mode)


TROUBLESHOOTING
---------------
"Cannot find FPVGate device"
→ Check USB cable connection
→ Look for COM port in Device Manager
→ Try different USB port

"Permission denied on serial port"
→ Close Arduino IDE, PuTTY, or other serial programs
→ Reconnect USB cable
→ Try switching to WiFi mode and back

App not starting
→ Make sure you extracted the entire folder
→ Run FPVGate.exe from extracted folder
→ Check Windows Defender/Antivirus isn't blocking


SUPPORT
-------
GitHub: https://github.com/LouisHitchcock/FPVGate
Documentation: https://github.com/LouisHitchcock/FPVGate/tree/main/docs
Issues: https://github.com/LouisHitchcock/FPVGate/issues


LICENSE
-------
MIT License - Open Source
See LICENSE file for details

