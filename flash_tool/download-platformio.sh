#!/bin/bash

# Download and setup PlatformIO Core for bundling with Electron app
# This script downloads PlatformIO Core and prepares it for bundling

set -e

RESOURCES_DIR="$(dirname "$0")/resources"
BIN_DIR="$RESOURCES_DIR/bin"
PLATFORMIO_DIR="$RESOURCES_DIR/platformio"

echo "📦 Setting up PlatformIO Core for bundling"
echo "=========================================="
echo ""

# Create directories
mkdir -p "$BIN_DIR"
mkdir -p "$PLATFORMIO_DIR"

# Check if Python 3 is available
if ! command -v python3 &> /dev/null; then
    echo "❌ Python 3 not found. Please install Python 3.7+"
    exit 1
fi

PYTHON_VERSION=$(python3 --version)
echo "✅ Found $PYTHON_VERSION"

# Install PlatformIO Core using pip
echo ""
echo "📥 Installing PlatformIO Core..."
python3 -m pip install --target "$PLATFORMIO_DIR" --upgrade platformio

if [ $? -eq 0 ]; then
    echo "✅ PlatformIO Core installed successfully"
else
    echo "❌ Failed to install PlatformIO Core"
    exit 1
fi

# Create wrapper scripts for each platform
echo ""
echo "📝 Creating wrapper scripts..."

# macOS wrapper
cat > "$BIN_DIR/platformio-macos" << 'EOF'
#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLATFORMIO_DIR="$SCRIPT_DIR/../platformio"
PYTHON="$PLATFORMIO_DIR/bin/python3"

# If Python executable doesn't exist, try system Python
if [ ! -f "$PYTHON" ]; then
    PYTHON="python3"
fi

exec "$PYTHON" -m platformio "$@"
EOF
chmod +x "$BIN_DIR/platformio-macos"

# Linux wrapper
cat > "$BIN_DIR/platformio-linux" << 'EOF'
#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLATFORMIO_DIR="$SCRIPT_DIR/../platformio"
PYTHON="$PLATFORMIO_DIR/bin/python3"

# If Python executable doesn't exist, try system Python
if [ ! -f "$PYTHON" ]; then
    PYTHON="python3"
fi

exec "$PYTHON" -m platformio "$@"
EOF
chmod +x "$BIN_DIR/platformio-linux"

# Windows wrapper (batch file)
cat > "$BIN_DIR/platformio-win64.bat" << 'EOF'
@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "PLATFORMIO_DIR=%SCRIPT_DIR%..\platformio"
set "PYTHON=%PLATFORMIO_DIR%\Scripts\python.exe"

REM If Python executable doesn't exist, try system Python
if not exist "%PYTHON%" (
    set "PYTHON=python"
)

"%PYTHON%" -m platformio %*
EOF

# Also create .exe wrapper for Windows (will be a batch file renamed)
cp "$BIN_DIR/platformio-win64.bat" "$BIN_DIR/platformio-win64.exe"

echo "✅ Wrapper scripts created"
echo ""
echo "📦 PlatformIO Core setup complete!"
echo ""
echo "Files created:"
ls -lh "$BIN_DIR"/platformio-*
echo ""
echo "🎉 PlatformIO Core is ready to be bundled with your Electron app."

