#!/bin/bash
# Unregister all runner instances locally

set -e

BASE_DIR="$HOME/actions-runner"

echo "Unregistering all runner instances..."

# Stop all processes first
pkill -f "actions-runner" 2>/dev/null || true
pkill -f "run.sh" 2>/dev/null || true
sleep 2

# Unregister each runner
for i in {1..8}; do
    RUNNER_DIR="${BASE_DIR}-${i}"
    if [ -d "$RUNNER_DIR" ] && [ -f "$RUNNER_DIR/.runner" ]; then
        echo "Unregistering runner $i..."
        cd "$RUNNER_DIR"
        ./config.sh remove --token "$(cat .runner | grep -oP '(?<="token":")[^"]*' || echo '')" 2>/dev/null || true
        # Alternative: just remove the .runner file
        rm -f .runner
        echo "  ✓ Runner $i unregistered"
    fi
done

# Also handle the main runner directory
if [ -d "$BASE_DIR" ] && [ -f "$BASE_DIR/.runner" ]; then
    echo "Unregistering main runner..."
    cd "$BASE_DIR"
    ./config.sh remove --token "$(cat .runner | grep -oP '(?<="token":")[^"]*' || echo '')" 2>/dev/null || true
    rm -f .runner
    echo "  ✓ Main runner unregistered"
fi

echo ""
echo "✓ Local unregistration complete!"
echo ""
echo "You still need to remove runners from GitHub:"
echo "https://github.com/RaceFPV/StarForgeOS/settings/actions/runners"
echo ""
echo "After removing from GitHub, you can run setup-parallel-runners.sh again."

