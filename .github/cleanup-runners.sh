#!/bin/bash
# Cleanup script to remove stale runner sessions and processes

set -e

BASE_DIR="$HOME/actions-runner"

echo "Cleaning up existing runners..."

# Stop all runner processes
echo "1. Stopping all runner processes..."
pkill -f "actions-runner" || true
pkill -f "run.sh" || true
sleep 2

# Stop systemd services if they exist
echo "2. Stopping systemd services..."
for i in {1..8}; do
    if systemctl is-active --quiet "actions-runner-${i}" 2>/dev/null; then
        sudo systemctl stop "actions-runner-${i}" || true
    fi
    if systemctl is-enabled --quiet "actions-runner-${i}" 2>/dev/null; then
        sudo systemctl disable "actions-runner-${i}" || true
    fi
done

# Remove lock files and stale sessions
echo "3. Removing lock files and stale sessions..."
for i in {1..8}; do
    RUNNER_DIR="${BASE_DIR}-${i}"
    if [ -d "$RUNNER_DIR" ]; then
        # Remove any lock files
        find "$RUNNER_DIR" -name "*.lock" -delete 2>/dev/null || true
        find "$RUNNER_DIR" -name ".runner" -delete 2>/dev/null || true
        # Remove work directory
        rm -rf "$RUNNER_DIR/_work"* 2>/dev/null || true
        echo "  Cleaned $RUNNER_DIR"
    fi
done

# Also clean the main runner directory if it exists
if [ -d "$BASE_DIR" ]; then
    find "$BASE_DIR" -name "*.lock" -delete 2>/dev/null || true
    rm -rf "$BASE_DIR/_work"* 2>/dev/null || true
    echo "  Cleaned $BASE_DIR"
fi

echo ""
echo "✓ Cleanup complete!"
echo ""
echo "IMPORTANT: You need to remove the runner from GitHub:"
echo "1. Go to: https://github.com/RaceFPV/StarForgeOS/settings/actions/runners"
echo "2. Click the '...' menu next to each runner"
echo "3. Click 'Remove runner'"
echo ""
echo "After removing from GitHub, you can run the setup script again."

