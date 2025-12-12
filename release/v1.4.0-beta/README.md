# FPVGate Desktop App v1.4.0

Windows desktop application for FPVGate lap timer with USB connectivity.

## Features

- Native USB Serial connectivity (zero-latency)
- Full feature parity with web interface
- Automatic COM port detection
- All v1.4.0 features:
  - Track Management System
  - Distance Tracking
  - Enhanced Race Editing
  - Modern Configuration UI
  - Race History with track association
  - LED controls and calibration

## System Requirements

- Windows 10 or later (64-bit)
- USB port for ESP32-S3 connection
- 100MB free disk space

## Installation

1. Extract `FPVGate_Desktop_v1.4.0_Windows_x64.zip`
2. Run `FPVGate.exe`
3. No installation required (portable app)

## Usage

### First Launch

1. Connect your ESP32-S3 FPVGate device via USB
2. Launch FPVGate.exe
3. Select your COM port from the dropdown
4. Click "Connect"

### COM Port Selection

The app automatically detects available serial ports. If your device doesn't appear:
- Check USB cable (must be data-capable, not charge-only)
- Check Windows Device Manager for COM port
- Try a different USB port
- Restart the ESP32-S3 device

### Features

All features from the web interface are available:
- Race timing with real-time lap display
- Track management and selection
- Distance tracking during races
- RSSI calibration wizard
- Configuration with 6 sections
- Race history with editing capabilities
- LED control and presets
- System diagnostics

### Connectivity

The desktop app uses **USB Serial CDC** for communication:
- Zero-latency local connection
- Faster than WiFi
- No network configuration required
- Works offline

## Troubleshooting

### App won't start
- Install Visual C++ Redistributable (included with Windows 10+)
- Run as Administrator
- Check antivirus isn't blocking

### Can't connect to device
- Verify device is powered on
- Check USB cable is data-capable
- Confirm COM port in Device Manager
- Try different USB port
- Restart ESP32-S3 device

### Features not working
- Ensure firmware is v1.4.0 or later
- Check USB connection is stable
- Try reconnecting

## Building from Source

To build the desktop app yourself:

```bash
cd electron
npm install
npm run build:win
```

Output will be in `electron/dist/`

## Version History

### v1.4.0 (December 10, 2024)
- Track Management System
- Distance Tracking
- Enhanced Race Editing
- Updated to match firmware v1.4.0

### v1.3.3
- Modern Configuration UI
- WebSocket Stability improvements

### v1.3.2
- Marshalling Mode
- LED settings persistence

### v1.3.0
- Initial desktop app release
- USB Serial CDC support
- Full web interface parity

## Support

- GitHub Issues: https://github.com/LouisHitchcock/FPVGate/issues
- GitHub Discussions: https://github.com/LouisHitchcock/FPVGate/discussions
- Documentation: https://github.com/LouisHitchcock/FPVGate/tree/main/docs

## License

MIT License - See LICENSE file for details
