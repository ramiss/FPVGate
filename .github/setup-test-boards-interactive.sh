#!/bin/bash
#
# Interactive setup script for ESP32 test boards
# Helps you assign names to each board interactively
#

set -e

TEST_DIR="/realworldtest"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

log() { echo -e "${GREEN}[INFO]${NC} $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Detect chip type
detect_chip() {
    local device=$1
    if command -v esptool.py &> /dev/null || command -v esptool &> /dev/null; then
        local esptool_cmd=$(command -v esptool.py || command -v esptool)
        local output=$($esptool_cmd --port "$device" flash_id 2>&1 || echo "")
        if echo "$output" | grep -qi "esp32c3"; then
            echo "esp32-c3"
        elif echo "$output" | grep -qi "esp32c6"; then
            echo "esp32-c6"
        elif echo "$output" | grep -qi "esp32s3"; then
            echo "esp32-s3"
        elif echo "$output" | grep -qi "esp32s2"; then
            echo "esp32-s2"
        elif echo "$output" | grep -qi "esp32[^-]"; then
            echo "esp32"
        else
            echo "unknown"
        fi
    else
        echo "unknown"
    fi
}

# Get USB serial number
get_serial() {
    local device=$1
    udevadm info -a -n "$device" 2>/dev/null | grep -i "ATTRS{serial}" | head -1 | cut -d'"' -f2 || echo "unknown"
}

log "StarForge Interactive Board Setup"
echo ""

# Find all serial devices
devices=($(find /dev -name "ttyUSB*" -o -name "ttyACM*" 2>/dev/null | sort))

if [ ${#devices[@]} -eq 0 ]; then
    error "No serial devices found"
    exit 1
fi

log "Found ${#devices[@]} device(s). Let's configure them:"
echo ""

# Create test directory
if [ ! -d "$TEST_DIR" ]; then
    log "Creating $TEST_DIR directory..."
    sudo mkdir -p "$TEST_DIR"
    sudo chown $USER:$USER "$TEST_DIR"
fi

# Clear existing symlinks
log "Clearing existing symlinks..."
rm -f "$TEST_DIR"/*

# Process each device
for i in "${!devices[@]}"; do
    device="${devices[$i]}"
    num=$((i + 1))
    
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    log "Device $num of ${#devices[@]}: $device"
    
    # Try to detect chip
    echo -n "  Detecting chip type... "
    chip_type=$(detect_chip "$device")
    if [ "$chip_type" != "unknown" ]; then
        echo -e "${GREEN}$chip_type${NC}"
    else
        echo -e "${YELLOW}unknown${NC} (couldn't detect)"
    fi
    
    # Get serial number
    serial=$(get_serial "$device")
    echo "  Serial: $serial"
    
    # Ask for name
    echo ""
    echo "Choose a name for this board:"
    echo "  1) esp32-c3-1"
    echo "  2) esp32-c3-2"
    echo "  3) esp32-c6-1"
    echo "  4) esp32-s3-1"
    echo "  5) esp32-s3-2"
    echo "  6) Custom name"
    echo ""
    read -p "Enter choice (1-6) or custom name: " choice
    
    case "$choice" in
        1) link_name="esp32-c3-1" ;;
        2) link_name="esp32-c3-2" ;;
        3) link_name="esp32-c6-1" ;;
        4) link_name="esp32-s3-1" ;;
        5) link_name="esp32-s3-2" ;;
        *)
            if [ -z "$choice" ]; then
                link_name="${chip_type}-$num"
            else
                link_name="$choice"
            fi
            ;;
    esac
    
    # Create symlink
    link_path="$TEST_DIR/$link_name"
    if [ -L "$link_path" ]; then
        warn "Symlink $link_path already exists, replacing..."
        rm "$link_path"
    fi
    
    log "Creating: $device -> $link_path"
    ln -s "$device" "$link_path"
    
    # Set permissions
    sudo chmod 666 "$device" 2>/dev/null || true
    sudo chown $USER:dialout "$device" 2>/dev/null || true
    
    echo ""
done

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
log "Setup complete! Symlinks created in $TEST_DIR:"
echo ""
ls -l "$TEST_DIR"
echo ""
log "You can now use these paths in your tests:"
echo "  TEST_PORT=/realworldtest/esp32-c3-1"
echo ""
warn "Note: These symlinks are temporary. For persistent symlinks,"
warn "set up udev rules using: ./setup-test-boards.sh --setup-udev"

