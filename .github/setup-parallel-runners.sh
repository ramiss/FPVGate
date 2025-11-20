#!/bin/bash
# Setup script for multiple GitHub Actions runner instances
# This allows parallel execution of jobs on a single machine

set -e

# Configuration
NUM_RUNNERS=8
BASE_DIR="$HOME/actions-runner"
REPO_URL="https://github.com/RaceFPV/StarForgeOS"
LABELS="self-hosted,Linux,X64,starforge,esp32hub"
RUNNER_VERSION="2.311.0"  # Update this to the latest version

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Setting up $NUM_RUNNERS parallel GitHub Actions runners...${NC}"

# Stop any existing runners
echo -e "${YELLOW}Stopping any existing runner processes...${NC}"
pkill -f "actions-runner" 2>/dev/null || true
pkill -f "run.sh" 2>/dev/null || true
sleep 2

# Check for systemd services
echo -e "${YELLOW}Checking for systemd services...${NC}"
for i in {1..8}; do
    if systemctl is-active --quiet "actions-runner-${i}" 2>/dev/null; then
        echo -e "${YELLOW}Stopping systemd service actions-runner-${i}...${NC}"
        sudo systemctl stop "actions-runner-${i}" 2>/dev/null || true
    fi
done

echo -e "${YELLOW}IMPORTANT: If you see 'A session for this runner already exists' errors,${NC}"
echo -e "${YELLOW}you need to remove the runners from GitHub first:${NC}"
echo -e "${YELLOW}https://github.com/RaceFPV/StarForgeOS/settings/actions/runners${NC}"
echo ""
read -p "Have you removed all existing runners from GitHub? (y/n) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo -e "${RED}Please remove the runners from GitHub first, then run this script again.${NC}"
    exit 1
fi

# Get registration token
echo -e "${GREEN}Step 1: Get registration token${NC}"
echo "Go to: https://github.com/RaceFPV/StarForgeOS/settings/actions/runners/new"
echo "Select 'Linux' and copy the registration token"
read -p "Paste the registration token here: " REGISTRATION_TOKEN

if [ -z "$REGISTRATION_TOKEN" ]; then
    echo -e "${RED}Error: Registration token is required${NC}"
    exit 1
fi

# Download runner (once, we'll copy it)
echo -e "${GREEN}Step 2: Downloading GitHub Actions runner...${NC}"
TEMP_DIR=$(mktemp -d)
cd "$TEMP_DIR"
RUNNER_URL="https://github.com/actions/runner/releases/download/v${RUNNER_VERSION}/actions-runner-linux-x64-${RUNNER_VERSION}.tar.gz"
curl -o actions-runner.tar.gz -L "$RUNNER_URL"
tar xzf actions-runner.tar.gz

# Create and configure each runner
echo -e "${GREEN}Step 3: Creating and configuring $NUM_RUNNERS runner instances...${NC}"

for i in $(seq 1 $NUM_RUNNERS); do
    RUNNER_DIR="${BASE_DIR}-${i}"
    
    echo -e "${GREEN}Setting up runner $i in $RUNNER_DIR...${NC}"
    
    # Create directory
    mkdir -p "$RUNNER_DIR"
    
    # Copy runner files
    cp -r "$TEMP_DIR"/* "$RUNNER_DIR/"
    
    # Configure runner
    cd "$RUNNER_DIR"
    
    # Remove existing .runner file if it exists (stale session)
    rm -f .runner
    
    # Configure with --replace flag to handle existing registrations
    ./config.sh --url "$REPO_URL" \
                --token "$REGISTRATION_TOKEN" \
                --name "esp32hub-${i}" \
                --labels "$LABELS" \
                --work "_work-${i}" \
                --replace \
                --unattended
    
    echo -e "${GREEN}✓ Runner $i configured${NC}"
done

# Cleanup temp directory
rm -rf "$TEMP_DIR"

# Create systemd service files
echo -e "${GREEN}Step 4: Creating systemd service files...${NC}"

for i in $(seq 1 $NUM_RUNNERS); do
    RUNNER_DIR="${BASE_DIR}-${i}"
    SERVICE_FILE="/etc/systemd/system/actions-runner-${i}.service"
    
    sudo tee "$SERVICE_FILE" > /dev/null <<EOF
[Unit]
Description=GitHub Actions Runner ${i}
After=network.target

[Service]
Type=simple
User=$USER
WorkingDirectory=$RUNNER_DIR
ExecStart=$RUNNER_DIR/run.sh
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF
    
    echo -e "${GREEN}✓ Service file created for runner $i${NC}"
done

# Create start/stop scripts
echo -e "${GREEN}Step 5: Creating management scripts...${NC}"

# Start script
cat > "$HOME/start-runners.sh" <<'EOF'
#!/bin/bash
# Start all runner instances

NUM_RUNNERS=8
BASE_DIR="$HOME/actions-runner"

for i in $(seq 1 $NUM_RUNNERS); do
    RUNNER_DIR="${BASE_DIR}-${i}"
    if [ -d "$RUNNER_DIR" ]; then
        echo "Starting runner $i..."
        cd "$RUNNER_DIR"
        ./run.sh &
        sleep 2
    fi
done

echo "All runners started. Check status with: ps aux | grep actions-runner"
EOF

# Stop script
cat > "$HOME/stop-runners.sh" <<'EOF'
#!/bin/bash
# Stop all runner instances

pkill -f "actions-runner"
echo "All runners stopped."
EOF

chmod +x "$HOME/start-runners.sh"
chmod +x "$HOME/stop-runners.sh"

echo -e "${GREEN}✓ Management scripts created${NC}"

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Setup complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "To start all runners:"
echo "  $HOME/start-runners.sh"
echo ""
echo "To stop all runners:"
echo "  $HOME/stop-runners.sh"
echo ""
echo "To use systemd services (recommended):"
echo "  sudo systemctl enable actions-runner-{1..8}"
echo "  sudo systemctl start actions-runner-{1..8}"
echo ""
echo "Check runner status in GitHub:"
echo "  https://github.com/RaceFPV/StarForgeOS/settings/actions/runners"
echo ""

