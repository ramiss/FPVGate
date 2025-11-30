# ESP32 Test Board Organization

## Overview

When you have multiple ESP32 boards connected to your test server, it's helpful to organize them with named symlinks. Instead of remembering `/dev/ttyUSB0` is your ESP32-C3, you can use `/realworldtest/esp32-c3-1`.

## Quick Start

### Option 1: Interactive Setup (Easiest)

```bash
cd StarForgeOS/.github
./setup-test-boards-interactive.sh
```

This will:
- Scan all connected boards
- Try to detect chip types automatically
- Ask you to name each board
- Create symlinks in `/realworldtest/`

### Option 2: Manual Setup

```bash
# Scan for boards
./setup-test-boards.sh --scan-only

# Create symlinks (auto-detects chip types)
./setup-test-boards.sh --create-links
```

## Persistent Setup with udev Rules

For symlinks that survive reboots and reconnections, use udev rules:

### Step 1: Get Board Serial Numbers

```bash
# For each board, get its unique serial number
udevadm info -a -n /dev/ttyUSB0 | grep ATTRS{serial}
```

### Step 2: Create udev Rules

Edit `/etc/udev/rules.d/99-starforge-test-boards.rules`:

```bash
sudo nano /etc/udev/rules.d/99-starforge-test-boards.rules
```

Add rules like:

```udev
# ESP32-C3 Board 1
SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", ATTRS{serial}=="YOUR_SERIAL_HERE", \
  SYMLINK+="realworldtest/esp32-c3-1", \
  MODE="0666", GROUP="dialout"

# ESP32-C3 Board 2
SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", ATTRS{serial}=="ANOTHER_SERIAL", \
  SYMLINK+="realworldtest/esp32-c3-2", \
  MODE="0666", GROUP="dialout"

# ESP32-S3 Board
SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", ATTRS{serial}=="ESP32S3_SERIAL", \
  SYMLINK+="realworldtest/esp32-s3-1", \
  MODE="0666", GROUP="dialout"
```

### Step 3: Apply Rules

```bash
# Reload udev rules
sudo udevadm control --reload-rules
sudo udevadm trigger

# Verify symlinks exist
ls -l /realworldtest/
```

## Using in Tests

Once set up, you can use named paths in your workflows:

### Environment Variables

```yaml
env:
  TEST_PORT: /realworldtest/esp32-c3-1
```

### In Scripts

```bash
export TEST_PORT=/realworldtest/esp32-c3-1
./test/run-headless-tests.sh
```

### Multiple Boards

```yaml
strategy:
  matrix:
    board: 
      - name: esp32-c3-1
        port: /realworldtest/esp32-c3-1
      - name: esp32-c3-2
        port: /realworldtest/esp32-c3-2
      - name: esp32-s3-1
        port: /realworldtest/esp32-s3-1
```

## Directory Structure

```
/realworldtest/
├── esp32-c3-1 -> /dev/ttyUSB0
├── esp32-c3-2 -> /dev/ttyUSB1
├── esp32-c6-1 -> /dev/ttyUSB2
├── esp32-s3-1 -> /dev/ttyUSB3
└── esp32-s3-2 -> /dev/ttyACM0
```

## Troubleshooting

### Symlinks not created

Check if `/realworldtest` directory exists:
```bash
sudo mkdir -p /realworldtest
sudo chown $USER:$USER /realworldtest
```

### Permissions denied

Add your user to dialout group:
```bash
sudo usermod -a -G dialout $USER
# Log out and back in, or:
newgrp dialout
```

### Symlinks disappear after reboot

Use udev rules (see Persistent Setup above) instead of manual symlinks.

### Can't detect chip type

Board may need to be in flash mode. Try:
```bash
# Put board in flash mode (usually hold BOOT button while resetting)
# Then try detection again
esptool.py --port /dev/ttyUSB0 flash_id
```

## Integration with GitHub Actions

Update your workflow to use the named paths:

```yaml
- name: Run tests on ESP32-C3-1
  env:
    TEST_PORT: /realworldtest/esp32-c3-1
    TEST_BOARD: esp32-c3
  run: ./test/run-headless-tests.sh
```

Or use matrix strategy for parallel testing:

```yaml
strategy:
  matrix:
    include:
      - port: /realworldtest/esp32-c3-1
        board: esp32-c3
      - port: /realworldtest/esp32-c3-2
        board: esp32-c3
      - port: /realworldtest/esp32-s3-1
        board: esp32-s3
```

