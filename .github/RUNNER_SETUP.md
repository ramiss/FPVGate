# Self-Hosted Runner Parallel Execution Setup

## Problem
GitHub Actions self-hosted runners process jobs sequentially (one at a time) by default, even on multi-core systems.

## Solution: Run Multiple Runner Instances

To enable parallel execution, run multiple instances of the runner on the same machine. Each instance can handle one job concurrently.

### Setup Steps

1. **Create multiple runner directories** (one per concurrent job):

```bash
# Create directories for 8 parallel runners (one per board type)
mkdir -p ~/actions-runner-{1..8}
cd ~/actions-runner-1
```

2. **Download and configure each runner**:

```bash
# For each runner instance (1-8):
cd ~/actions-runner-1
curl -o actions-runner-linux-x64-2.311.0.tar.gz -L https://github.com/actions/runner/releases/download/v2.311.0/actions-runner-linux-x64-2.311.0.tar.gz
tar xzf ./actions-runner-linux-x64-2.311.0.tar.gz

# Configure with same labels but different names
./config.sh --url https://github.com/RaceFPV/StarForgeOS \
  --token <YOUR_TOKEN> \
  --name "esp32hub-1" \
  --labels "self-hosted,Linux,X64,starforge,esp32hub" \
  --work _work-1

# Repeat for runners 2-8 with different names and work folders
```

3. **Create systemd service files** for each runner:

```bash
# Example for runner 1: /etc/systemd/system/actions-runner-1.service
[Unit]
Description=GitHub Actions Runner 1
After=network.target

[Service]
Type=simple
User=your-user
WorkingDirectory=/home/your-user/actions-runner-1
ExecStart=/home/your-user/actions-runner-1/run.sh
Restart=always

[Install]
WantedBy=multi-user.target
```

4. **Start all runners**:

```bash
sudo systemctl enable actions-runner-1
sudo systemctl start actions-runner-1
# Repeat for runners 2-8
```

### Alternative: Single Runner with Script

If you prefer a simpler approach, you can create a script that manages multiple runner processes:

```bash
#!/bin/bash
# run-multiple-runners.sh

NUM_RUNNERS=8
RUNNER_DIR=~/actions-runner

for i in $(seq 1 $NUM_RUNNERS); do
  cd "$RUNNER_DIR-$i"
  ./run.sh &
done

wait
```

### Recommended Configuration

For a 16-core system building 8 board types:
- **8 runner instances**: One per board type (matches `max-parallel: 8`)
- Each runner handles one job at a time
- All 8 can run in parallel, utilizing your 16 cores effectively

### Monitoring

Check runner status:
```bash
# List all running instances
ps aux | grep actions-runner

# Check GitHub Actions UI
# Settings → Actions → Runners
# You should see 8 runners online
```

### Notes

- Each runner instance uses its own `_work` directory
- PlatformIO builds are CPU-intensive, so 8 parallel builds on 16 cores is reasonable
- Monitor system resources (CPU, memory, disk I/O) to ensure optimal performance
- You can adjust the number of runners based on your system's capacity

