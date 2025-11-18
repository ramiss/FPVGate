#!/usr/bin/env node

/**
 * Setup script to download and prepare PlatformIO Core for bundling
 * This runs before the Electron app is built
 */

const { spawn, execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

const RESOURCES_DIR = path.join(__dirname, '..', 'resources');
const BIN_DIR = path.join(RESOURCES_DIR, 'bin');
const PLATFORMIO_DIR = path.join(RESOURCES_DIR, 'platformio');

// Ensure directories exist
if (!fs.existsSync(BIN_DIR)) {
  fs.mkdirSync(BIN_DIR, { recursive: true });
}
if (!fs.existsSync(PLATFORMIO_DIR)) {
  fs.mkdirSync(PLATFORMIO_DIR, { recursive: true });
}

// Check if PlatformIO is already installed
const platformioExists = fs.existsSync(path.join(PLATFORMIO_DIR, 'platformio', '__main__.py'));

if (platformioExists) {
  console.log('✅ PlatformIO Core already installed');
  createWrappers();
  process.exit(0);
}

console.log('📦 Installing PlatformIO Core...');
console.log('   This may take a few minutes on first run...\n');

// Find Python command
let pythonCmd = 'python3';
try {
  execSync('python3 --version', { stdio: 'ignore' });
} catch {
  try {
    execSync('python --version', { stdio: 'ignore' });
    pythonCmd = 'python';
  } catch {
    console.error('❌ Python not found. Please install Python 3.7+');
    console.error('   Download from: https://www.python.org/downloads/');
    process.exit(1);
  }
}

// Install PlatformIO Core
const pipInstall = spawn(pythonCmd, [
  '-m', 'pip', 'install',
  '--target', PLATFORMIO_DIR,
  '--upgrade',
  'platformio'
], {
  stdio: 'inherit',
  shell: process.platform === 'win32'
});

pipInstall.on('close', (code) => {
  if (code === 0) {
    console.log('\n✅ PlatformIO Core installed successfully');
    createWrappers();
  } else {
    console.error('\n❌ Failed to install PlatformIO Core');
    process.exit(1);
  }
});

pipInstall.on('error', (err) => {
  console.error('❌ Error installing PlatformIO:', err.message);
  process.exit(1);
});

function createWrappers() {
  console.log('\n📝 Creating wrapper scripts...');
  
  // macOS/Linux wrapper
  // Note: Use string concatenation to avoid template literal interpolation of ${BASH_SOURCE[0]}
  const unixWrapper = '#!/bin/bash\n' +
'SCRIPT_DIR="$(cd "$(dirname "${' + 'BASH_SOURCE[0]}")" && pwd)"\n' +
'PLATFORMIO_DIR="$SCRIPT_DIR/../platformio"\n' +
'\n' +
'# Use system Python with bundled PlatformIO package\n' +
'if command -v python3 &> /dev/null; then\n' +
'    PYTHON="python3"\n' +
'elif command -v python &> /dev/null; then\n' +
'    PYTHON="python"\n' +
'else\n' +
'    echo "Error: Python not found. Please install Python 3.7+" >&2\n' +
'    exit 1\n' +
'fi\n' +
'\n' +
'# Add bundled PlatformIO to Python path and run\n' +
'export PYTHONPATH="$PLATFORMIO_DIR:$PYTHONPATH"\n' +
'exec "$PYTHON" -m platformio "$@"\n';

  // Windows wrapper
  const winWrapper = `@echo off
setlocal

REM Set UTF-8 encoding for console output (fixes Unicode errors)
chcp 65001 >nul 2>&1

set "SCRIPT_DIR=%~dp0"
set "PLATFORMIO_DIR=%SCRIPT_DIR%..\\platformio"

REM Use system Python with bundled PlatformIO package
where python >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    set "PYTHON=python"
) else (
    echo Error: Python not found. Please install Python 3.7+
    exit /b 1
)

REM Add bundled PlatformIO to Python path
set "PYTHONPATH=%PLATFORMIO_DIR%;%PYTHONPATH%"

REM Set Python UTF-8 encoding environment variables
set "PYTHONIOENCODING=utf-8"
set "PYTHONUTF8=1"

"%PYTHON%" -m platformio %*
`;

  // Write macOS wrapper
  const macWrapperPath = path.join(BIN_DIR, 'platformio-macos');
  fs.writeFileSync(macWrapperPath, unixWrapper);
  if (process.platform !== 'win32') {
    fs.chmodSync(macWrapperPath, 0o755);
  }

  // Write Linux wrapper
  const linuxWrapperPath = path.join(BIN_DIR, 'platformio-linux');
  fs.writeFileSync(linuxWrapperPath, unixWrapper);
  if (process.platform !== 'win32') {
    fs.chmodSync(linuxWrapperPath, 0o755);
  }

  // Write Windows wrapper
  const winWrapperPath = path.join(BIN_DIR, 'platformio-win64.bat');
  fs.writeFileSync(winWrapperPath, winWrapper);

  console.log('✅ Wrapper scripts created');
  console.log('\n📦 PlatformIO Core setup complete!');
  console.log('   Files ready for bundling:\n');
  
  if (fs.existsSync(macWrapperPath)) {
    console.log(`   - ${path.relative(process.cwd(), macWrapperPath)}`);
  }
  if (fs.existsSync(linuxWrapperPath)) {
    console.log(`   - ${path.relative(process.cwd(), linuxWrapperPath)}`);
  }
  if (fs.existsSync(winWrapperPath)) {
    console.log(`   - ${path.relative(process.cwd(), winWrapperPath)}`);
  }
  console.log(`   - ${path.relative(process.cwd(), PLATFORMIO_DIR)}/ (PlatformIO package)\n`);
}

