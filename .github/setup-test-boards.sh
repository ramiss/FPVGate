#!/bin/bash
#
# Setup script for organizing ESP32 test boards with named symlinks
# 
# This script helps organize multiple ESP32 boards connected via USB by creating
# symbolic links with meaningful names like /realworldtest/esp32-c3-1
#
# Usage:
#   ./setup-test-boards.sh [--scan-only] [--create-links] [--setup-udev]
#
# Options:
#   --scan-only      Just scan and list connected boards
#   --create-links   Create symlinks in /realworldtest/ (requires sudo for udev rules)
#   --setup-udev     Install udev rules for automatic symlink creation
#

set -e

TEST_DIR="/realworldtest"
UDEV_RULES_FILE="/etc/udev/rules.d/99-starforge-test-boards.rules"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

# Scan for connected ESP32 boards
scan_boards() {
    log "Scanning for connected ESP32 boards..."
    echo ""
    
    # Find all serial devices
    local devices=($(find /dev -name "ttyUSB*" -o -name "ttyACM*" 2>/dev/null | sort))
    
    if [ ${#devices[@]} -eq 0 ]; then
        warn "No serial devices found. Connect ESP32 boards and try again."
        return 1
    fi
    
    info "Found ${#devices[@]} serial device(s):"
    echo ""
    
    local board_num=1
    for device in "${devices[@]}"; do
        # Get USB device info
        local device_path=$(readlink -f "$device")
        local usb_device=$(basename "$device_path" | sed 's/tty.*//')
        
        # Try to get USB vendor/product info
        local vendor_id=""
        local product_id=""
        local manufacturer=""
        local product=""
        
        # Get vendor/product IDs from udev
        if command -v udevadm &> /dev/null; then
            local udev_info=$(udevadm info --name="$device" 2>/dev/null || true)
            vendor_id=$(echo "$udev_info" | grep -i "ID_VENDOR_ID" | cut -d'=' -f2 | head -1)
            product_id=$(echo "$udev_info" | grep -i "ID_MODEL_ID" | cut -d'=' -f2 | head -1)
            manufacturer=$(echo "$udev_info" | grep -i "ID_VENDOR=" | cut -d'=' -f2 | head -1)
            product=$(echo "$udev_info" | grep -i "ID_MODEL=" | cut -d'=' -f2 | head -1)
        fi
        
        # Try to detect chip type using esptool
        local chip_type="unknown"
        if command -v esptool.py &> /dev/null || command -v esptool &> /dev/null; then
            local esptool_cmd=$(command -v esptool.py || command -v esptool)
            if timeout 2 $esptool_cmd --port "$device" flash_id &>/dev/null; then
                local chip_output=$($esptool_cmd --port "$device" flash_id 2>&1 | grep -iE "chip|esp32" || true)
                if echo "$chip_output" | grep -qi "esp32c3"; then
                    chip_type="esp32-c3"
                elif echo "$chip_output" | grep -qi "esp32c6"; then
                    chip_type="esp32-c6"
                elif echo "$chip_output" | grep -qi "esp32s3"; then
                    chip_type="esp32-s3"
                elif echo "$chip_output" | grep -qi "esp32s2"; then
                    chip_type="esp32-s2"
                elif echo "$chip_output" | grep -qi "esp32[^-]"; then
                    chip_type="esp32"
                fi
            fi
        fi
        
        # Format output
        printf "  %d. ${BLUE}%s${NC}\n" "$board_num" "$device"
        printf "     VID: %s  PID: %s\n" "${vendor_id:-unknown}" "${product_id:-unknown}"
        printf "     Manufacturer: %s\n" "${manufacturer:-unknown}"
        printf "     Product: %s\n" "${product:-unknown}"
        
        if [ "$chip_type" != "unknown" ]; then
            printf "     ${GREEN}Detected Chip: %s${NC}\n" "$chip_type"
        else
            printf "     ${YELLOW}Chip: Not detected (may need to be in flash mode)${NC}\n"
        fi
        
        echo ""
        board_num=$((board_num + 1))
    done
    
    return 0
}

# Detect chip type for a device
detect_chip_type() {
    local device=$1
    if command -v esptool.py &> /dev/null || command -v esptool &> /dev/null; then
        local esptool_cmd=$(command -v esptool.py || command -v esptool)
        local output=$($esptool_cmd --port "$device" flash_id 2>&1 || true)
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

# Create symlinks manually
create_links() {
    log "Creating symlinks in $TEST_DIR..."
    
    # Create test directory
    if [ ! -d "$TEST_DIR" ]; then
        log "Creating $TEST_DIR directory..."
        sudo mkdir -p "$TEST_DIR"
        sudo chown $USER:$USER "$TEST_DIR"
    fi
    
    # Scan devices
    local devices=($(find /dev -name "ttyUSB*" -o -name "ttyACM*" 2>/dev/null | sort))
    
    if [ ${#devices[@]} -eq 0 ]; then
        error "No serial devices found"
        return 1
    fi
    
    # Create symlinks for each device
    local chip_counts=()
    for device in "${devices[@]}"; do
        local chip_type=$(detect_chip_type "$device")
        
        if [ "$chip_type" == "unknown" ]; then
            warn "Could not detect chip type for $device, using 'unknown'"
            chip_type="unknown"
        fi
        
        # Count occurrences of this chip type
        local count=$(echo "${chip_counts[@]}" | grep -o "$chip_type" | wc -l)
        count=$((count + 1))
        chip_counts+=("$chip_type")
        
        # Create symlink name
        local link_name="$chip_type-$count"
        local link_path="$TEST_DIR/$link_name"
        
        # Remove existing symlink if it exists
        if [ -L "$link_path" ]; then
            log "Removing existing symlink: $link_path"
            rm "$link_path"
        fi
        
        # Create new symlink
        log "Linking $device -> $link_path"
        ln -s "$device" "$link_path"
        
        # Set permissions
        sudo chmod 666 "$device" 2>/dev/null || true
        sudo chown $USER:dialout "$device" 2>/dev/null || true
    done
    
    log "Symlinks created in $TEST_DIR:"
    ls -l "$TEST_DIR"
    
    return 0
}

# Setup udev rules for automatic symlink creation
setup_udev() {
    log "Setting up udev rules for automatic symlink creation..."
    
    if [ ! -w "/etc/udev" ]; then
        error "This operation requires sudo privileges"
        return 1
    fi
    
    # Create udev rules file
    log "Creating udev rules file: $UDEV_RULES_FILE"
    
    sudo tee "$UDEV_RULES_FILE" > /dev/null <<'EOF'
# StarForge Test Board Symlinks
# Automatically creates symlinks for ESP32 boards in /realworldtest/
#
# To add a new board, get its serial number:
#   udevadm info -a -n /dev/ttyUSB0 | grep ATTRS{serial}
#
# Then add a rule like:
#   SUBSYSTEM=="tty", ATTRS{serial}=="YOUR_SERIAL", SYMLINK+="realworldtest/esp32-c3-1", MODE="0666", GROUP="dialout"

# Example rules (replace with your actual serial numbers):
# SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", ATTRS{serial}=="YOUR_SERIAL_HERE", SYMLINK+="realworldtest/esp32-c3-1", MODE="0666", GROUP="dialout"

# Generic rule for all Silicon Labs USB-to-Serial (ESP32 common)
SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", MODE="0666", GROUP="dialout"

# Generic rule for CH340 (ESP32 common)
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", MODE="0666", GROUP="dialout"

# Generic rule for WCH (ESP32 common)
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", MODE="0666", GROUP="dialout"
EOF

    log "Udev rules file created. You'll need to:"
    log "1. Get serial numbers for each board:"
    echo "   udevadm info -a -n /dev/ttyUSB0 | grep ATTRS{serial}"
    log "2. Edit $UDEV_RULES_FILE and add specific rules for each board"
    log "3. Reload udev rules:"
    echo "   sudo udevadm control --reload-rules"
    echo "   sudo udevadm trigger"
    
    return 0
}

# Show help
show_help() {
    cat << EOF
StarForge Test Board Setup Script

This script helps organize multiple ESP32 boards by creating named symlinks.

Usage:
  $0 [OPTIONS]

Options:
  --scan-only      Scan and list all connected ESP32 boards
  --create-links   Create symlinks in $TEST_DIR (requires manual chip detection)
  --setup-udev     Create udev rules template for automatic symlink creation
  --help           Show this help message

Examples:
  # Scan for connected boards
  $0 --scan-only
  
  # Create manual symlinks (will try to detect chip types)
  $0 --create-links
  
  # Setup udev rules for automatic symlinks (requires sudo)
  sudo $0 --setup-udev

Manual Setup:
  1. Get serial number for each board:
     udevadm info -a -n /dev/ttyUSB0 | grep ATTRS{serial}
  
  2. Create udev rule in /etc/udev/rules.d/99-starforge-test-boards.rules:
     SUBSYSTEM=="tty", ATTRS{serial}=="YOUR_SERIAL", SYMLINK+="realworldtest/esp32-c3-1", MODE="0666", GROUP="dialout"
  
  3. Reload rules:
     sudo udevadm control --reload-rules
     sudo udevadm trigger

EOF
}

# Main
main() {
    case "${1:-}" in
        --scan-only)
            scan_boards
            ;;
        --create-links)
            scan_boards
            echo ""
            read -p "Create symlinks for these devices? (y/n) " -n 1 -r
            echo
            if [[ $REPLY =~ ^[Yy]$ ]]; then
                create_links
            fi
            ;;
        --setup-udev)
            setup_udev
            ;;
        --help|-h)
            show_help
            ;;
        "")
            log "StarForge Test Board Setup"
            echo ""
            show_help
            echo ""
            scan_boards
            ;;
        *)
            error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
}

main "$@"

