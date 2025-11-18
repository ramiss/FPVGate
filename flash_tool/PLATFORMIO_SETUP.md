# PlatformIO Core Bundling

This document explains how PlatformIO Core is bundled with the StarForge Flash Tool.

## Overview

PlatformIO Core is bundled with the Electron app so users don't need to install it separately. The bundling process:

1. Downloads PlatformIO Core using `pip install --target`
2. Creates wrapper scripts for each platform
3. Bundles everything with the Electron app

## Setup

PlatformIO Core is automatically set up when you run:

```bash
npm install
```

Or manually:

```bash
npm run setup-platformio
```

This will:
- Install PlatformIO Core into `resources/platformio/`
- Create wrapper scripts in `resources/bin/`

## Wrapper Scripts

The wrapper scripts use system Python (which most users have) with the bundled PlatformIO package:

- **macOS/Linux**: `platformio-macos`, `platformio-linux` (bash scripts)
- **Windows**: `platformio-win64.bat` (batch file)

The wrappers:
1. Find system Python (`python3` or `python`)
2. Add the bundled PlatformIO to `PYTHONPATH`
3. Run `python -m platformio` with the provided arguments

## Requirements

### For Building (Developers)

- Python 3.7+ installed and in PATH
- pip (usually comes with Python)

### For End Users

- Python 3.7+ installed and in PATH
  - macOS: Usually pre-installed, or install from python.org
  - Windows: Download from python.org (check "Add to PATH" during install)
  - Linux: Usually pre-installed, or `sudo apt install python3`

**Note**: Most users already have Python installed, so this is usually not an issue.

## Building

When you build the Electron app, PlatformIO Core is automatically included:

```bash
npm run build
```

The `prebuild` script runs `setup-platformio` automatically, ensuring PlatformIO is bundled.

## File Structure

```
resources/
├── bin/
│   ├── platformio-macos      # macOS wrapper
│   ├── platformio-linux       # Linux wrapper
│   └── platformio-win64.bat   # Windows wrapper
└── platformio/                # PlatformIO Core package
    ├── platformio/
    ├── click/
    ├── ... (other dependencies)
```

## Troubleshooting

### "Python not found" error

**For developers**: Make sure Python 3.7+ is installed and in your PATH.

**For end users**: Install Python from https://www.python.org/downloads/
- On Windows: Check "Add Python to PATH" during installation

### PlatformIO not found after building

1. Make sure `npm run setup-platformio` ran successfully
2. Check that `resources/platformio/platformio/__main__.py` exists
3. Check that wrapper scripts exist in `resources/bin/`

### PlatformIO works in dev but not in packaged app

- Make sure `extraResources` in `package.json` includes both `platformio-*` files and the `platformio/` directory
- Rebuild the app: `npm run build`

## Alternative: Fully Standalone (Future)

For a fully standalone solution (no Python required), we could:
1. Bundle a portable Python runtime (~50-100MB)
2. Install PlatformIO into that runtime
3. Use it from the app

This would make the app larger but eliminate the Python requirement. Currently not implemented.

