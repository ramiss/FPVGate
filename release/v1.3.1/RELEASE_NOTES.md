# FPVGate v1.3.1 Release Notes

**Release Date:** December 8, 2025

This is a maintenance release with important bug fixes and calibration wizard improvements.

## Bug Fixes

### Race History Storage Fixed
- **Fixed race history not saving** - Race history was not being initialized with the storage backend, preventing races from being saved to SD card or LittleFS
- Races are now properly saved after each race session and persisted across reboots
- Added automatic reload of race history from SD card when SD is mounted

### Calibration Wizard Improvements
- **Simplified 3-peak marking** - Wizard now only requires marking the 3 highest peaks (one per lap), instead of 6 entry/exit points
- **Improved threshold calculation** - Thresholds are now calculated as drops from peak rather than rises from baseline:
  - Entry RSSI: 25% down from peak (catches rising edge well into the spike)
  - Exit RSSI: 40% down from peak (catches falling edge, still above baseline)
  - Typically results in ~20-30 RSSI difference between entry and exit thresholds
- **Enhanced visual display** - Added smooth 15-point moving average filter and filled area under the calibration chart for easier peak identification
- **Better UI instructions** - Updated wizard text to clarify that only peak points need to be marked

## Technical Details

### Changed Files
- `src/main.cpp` - Added race history initialization
- `data/index.html` - Updated calibration wizard UI instructions
- `data/script.js` - Improved threshold calculation and visual smoothing
- `lib/RACEHISTORY/*` - Race history functionality now properly integrated

### Calibration Algorithm
The new threshold calculation uses peak-relative values:
```
baseline = min(all RSSI values)
avgPeak = average of 3 marked peaks
peakRange = avgPeak - baseline

entryRssi = avgPeak - (peakRange * 0.25)
exitRssi = avgPeak - (peakRange * 0.40)
```

This provides more intuitive and accurate threshold values that work better across different signal strengths and environments.

## Upgrading from v1.3.0

1. Flash the new firmware as normal
2. No configuration changes required
3. Existing race history will be preserved
4. Re-run calibration wizard to take advantage of improved threshold calculation

## What's Included

- Firmware binary for ESP32-S3
- Firmware binary for ESP32-C3
- Firmware binary for ESP32
- Updated web interface (LittleFS image)
- Source code

## Credits

Thank you to all contributors and testers who helped identify and fix these issues!

## Full Changelog

See [CHANGELOG.md](../../CHANGELOG.md) for complete version history.
