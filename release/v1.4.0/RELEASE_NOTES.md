# FPVGate v1.4.0 - Track Management & Distance Tracking

Release Date: December 10, 2024

## What's New

### Track Management System

FPVGate now includes a complete track management system, allowing you to create, edit, and manage track profiles directly from the web interface.

**Key Features:**
- Create track profiles with name, tags, distance, and notes
- Upload track images for visual reference
- Store up to 50 tracks on SD card or LittleFS
- Select active track before racing (selection persists across reboots)
- Track information displayed during races
- Full CRUD operations via web interface

**Track Storage:**
- Individual track files: `/tracks/track_<timestamp>.json`
- Track images: `/tracks/images/track_<id>.jpg`
- Automatic directory creation on first use

### Distance Tracking

Real-time distance tracking during races provides accurate statistics for training and analysis.

**Features:**
- Live distance display updates every 100ms during race
- Shows current lap distance (when track selected)
- Total distance travelled counter
- Distance remaining for finite lap races
- Distance data stored with race history
- Per-lap distance calculations in race details

**Display Format:**
```
Current Lap: 12.34s | 125/500m
Total Distance: 1250m
Distance Remaining: 1250m
```

### Enhanced Race Editing

Expanded race editing capabilities beyond the marshalling mode introduced in v1.3.2.

**New Capabilities:**
- Edit race metadata (name, tags, notes, track association, distance)
- Edit individual lap times
- Drag-and-drop lap reordering
- Separate API endpoints for metadata vs lap changes
- Tabbed edit interface (Info tab and Laps tab)
- Real-time statistics recalculation
- Better validation and confirmation dialogs

### API Enhancements

**New Track Management Endpoints:**
- `GET /tracks` - List all tracks
- `POST /tracks/create` - Create new track
- `POST /tracks/update` - Update existing track
- `POST /tracks/delete` - Delete track
- `POST /tracks/select` - Set active track
- `GET /tracks/image/<id>` - Retrieve track image

**New Distance Tracking Endpoint:**
- `GET /timer/distance` - Get current distance statistics

**Enhanced Race Endpoints:**
- `POST /races/update` - Update race metadata only
- `POST /races/updateLaps` - Update race lap times only

## Improvements

### Race History
- Track name and distance displayed in history list
- Track association with races (trackId, trackName)
- Total distance field added to race data
- Notes field for race-specific comments
- Enhanced race details view with distance stats
- Track filter in history search

### Configuration
- Selected track ID stored in EEPROM (persists across reboots)
- Track selector dropdown in Configuration modal
- Backwards compatible with existing configurations

### User Interface
- Current lap time shown during active race
- Distance counter in race interface (when track selected)
- Improved race edit modal with tabbed interface
- Track management section in Configuration modal
- Visual lap editor with inline controls

## Bug Fixes

- Fixed race data not including distance information
- Fixed per-lap distance calculation display
- Fixed track selection not persisting after reboot
- Improved race saving reliability

## Technical Changes

### New Components
- `lib/TRACKMANAGER/` - Complete track management library
- Track CRUD operations with SD card storage
- LapTimer distance tracking integration

### Data Structures
- Track struct with metadata fields
- Enhanced Race struct with distance and track fields
- Config expanded with selectedTrackId

### Frontend
- Track management UI components
- Distance polling system (100ms refresh)
- Enhanced race edit modal
- Track selector dropdowns

## Compatibility

- Backwards compatible with v1.3.x races (null distance/track values)
- Config version remains v3
- SD card preferred, LittleFS fallback maintained
- All existing features fully functional

## Installation

### Firmware Flash (ESP32-S3)

```bash
esptool.py --chip esp32s3 --port COM3 write_flash -z \
  0x0 bootloader.bin \
  0x8000 partitions.bin \
  0x10000 firmware.bin \
  0x410000 littlefs.bin
```

### Desktop App

Download and install the Electron app for Windows, macOS, or Linux.

**Windows:**
- Download `FPVGate_Desktop_v1.4.0_Windows_x64.zip`
- Extract and run `FPVGate.exe`
- Connect via USB and select COM port

## Documentation

All documentation has been updated for v1.4.0:

- [Getting Started Guide](../../docs/GETTING_STARTED.md) - Setup and configuration
- [User Guide](../../docs/USER_GUIDE.md) - Complete feature walkthrough
- [Hardware Guide](../../docs/HARDWARE_GUIDE.md) - Wiring and components
- [Features Guide](../../docs/FEATURES.md) - In-depth technical docs
- [Development Guide](../../docs/DEVELOPMENT.md) - Building from source

## Upgrade Notes

### From v1.3.x
- No breaking changes
- Existing races remain accessible
- Config automatically migrated
- Track management is optional (can continue without using tracks)

### First Boot After Update
- Track directories created automatically on SD card
- Selected track defaults to "None"
- All existing features work as before
- No recalibration required

## Known Issues

- Track images limited to JPEG format
- Maximum 50 tracks (configurable in trackmanager.h)
- Distance tracking requires track selection

## Credits

FPVGate is a heavily modified fork of PhobosLT by phobos-. Timing logic inspired by RotorHazard.

## Support

- GitHub Issues: https://github.com/LouisHitchcock/FPVGate/issues
- GitHub Discussions: https://github.com/LouisHitchcock/FPVGate/discussions

## License

MIT License - See LICENSE file for details

---

**Full Changelog:** https://github.com/LouisHitchcock/FPVGate/blob/main/CHANGELOG.md
