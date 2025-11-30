#!/bin/bash
#
# Headless Test Runner Script for StarForge Flash Tool
# 
# This script sets up a virtual display and runs the test suite
# against real hardware connected via USB.
#
# Prerequisites:
#   - Xvfb installed: sudo apt-get install xvfb
#   - Node.js and npm installed
#   - esptool installed: pip install esptool
#   - ESP32 board connected via USB
#
# Usage:
#   ./test/run-headless-tests.sh
#
# Environment Variables:
#   TEST_PORT - Serial port to test (default: auto-detect)
#   TEST_BOARD - Board type (default: auto-detect)
#   TEST_FIRMWARE_DIR - Directory with test firmware
#   DISPLAY - Override display (default: :99)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
XVFB_DISPLAY="${DISPLAY:-:99}"
XVFB_PID_FILE="/tmp/xvfb-flash-test-${XVFB_DISPLAY//:/-}.pid"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1" >&2
}

warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

# Check dependencies
check_dependencies() {
    log "Checking dependencies..."
    
    local missing=0
    
    if ! command -v node &> /dev/null; then
        error "Node.js not found. Install from https://nodejs.org"
        missing=1
    fi
    
    if ! command -v Xvfb &> /dev/null; then
        warning "Xvfb not found. Install with: sudo apt-get install xvfb"
        warning "Tests may fail without a virtual display"
        missing=1
    fi
    
    if ! command -v esptool.py &> /dev/null && ! command -v esptool &> /dev/null; then
        warning "esptool not found. Install with: pip install esptool"
    fi
    
    if [ $missing -eq 1 ]; then
        error "Missing required dependencies"
        exit 1
    fi
    
    log "Dependencies OK"
}

# Start Xvfb if not already running
start_xvfb() {
    log "Setting up virtual display ${XVFB_DISPLAY}..."
    
    # Check if Xvfb is already running on this display
    if xdpyinfo -display "${XVFB_DISPLAY}" &> /dev/null; then
        log "Display ${XVFB_DISPLAY} already exists"
        export DISPLAY="${XVFB_DISPLAY}"
        return 0
    fi
    
    # Check if there's a PID file from a previous run
    if [ -f "$XVFB_PID_FILE" ]; then
        local old_pid=$(cat "$XVFB_PID_FILE")
        if ps -p "$old_pid" > /dev/null 2>&1; then
            log "Cleaning up stale Xvfb process (PID: $old_pid)"
            kill "$old_pid" 2>/dev/null || true
        fi
        rm -f "$XVFB_PID_FILE"
    fi
    
    # Start Xvfb
    log "Starting Xvfb on display ${XVFB_DISPLAY}..."
    Xvfb "${XVFB_DISPLAY}" -screen 0 1024x768x24 -ac +extension RANDR > /dev/null 2>&1 &
    local xvfb_pid=$!
    echo "$xvfb_pid" > "$XVFB_PID_FILE"
    
    # Wait for Xvfb to be ready
    local retries=10
    while [ $retries -gt 0 ]; do
        if xdpyinfo -display "${XVFB_DISPLAY}" &> /dev/null; then
            log "Xvfb started successfully (PID: $xvfb_pid)"
            export DISPLAY="${XVFB_DISPLAY}"
            return 0
        fi
        sleep 0.5
        retries=$((retries - 1))
    done
    
    error "Failed to start Xvfb"
    rm -f "$XVFB_PID_FILE"
    return 1
}

# Stop Xvfb
stop_xvfb() {
    if [ -f "$XVFB_PID_FILE" ]; then
        local xvfb_pid=$(cat "$XVFB_PID_FILE")
        if ps -p "$xvfb_pid" > /dev/null 2>&1; then
            log "Stopping Xvfb (PID: $xvfb_pid)"
            kill "$xvfb_pid" 2>/dev/null || true
        fi
        rm -f "$XVFB_PID_FILE"
    fi
}

# Cleanup function
cleanup() {
    log "Cleaning up..."
    stop_xvfb
}

# Trap to ensure cleanup on exit
trap cleanup EXIT INT TERM

# Main execution
main() {
    log "StarForge Flash Tool - Headless Test Runner"
    log "==========================================="
    
    # Change to project directory
    cd "$PROJECT_DIR"
    
    # Check dependencies
    check_dependencies
    
    # Install npm dependencies if needed
    if [ ! -d "node_modules" ]; then
        log "Installing npm dependencies..."
        npm install
    fi
    
    # Start virtual display
    if [ -z "$DISPLAY" ] || ! xdpyinfo -display "$DISPLAY" &> /dev/null; then
        start_xvfb
    else
        log "Using existing display: $DISPLAY"
    fi
    
    # Check for serial port permissions
    if [ -n "$TEST_PORT" ] && [ ! -r "$TEST_PORT" ]; then
        warning "Port $TEST_PORT may not be readable"
        warning "Add your user to dialout group: sudo usermod -a -G dialout \$USER"
    fi
    
    # Run tests
    log "Running test suite..."
    log "  TEST_PORT: ${TEST_PORT:-auto-detect}"
    log "  TEST_BOARD: ${TEST_BOARD:-auto-detect}"
    log "  TEST_FIRMWARE_DIR: ${TEST_FIRMWARE_DIR:-./test-firmware}"
    log "  DISPLAY: $DISPLAY"
    
    node test/flash-test-suite.js
    
    exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        log "All tests passed!"
    else
        error "Some tests failed (exit code: $exit_code)"
    fi
    
    return $exit_code
}

# Run main function
main "$@"

