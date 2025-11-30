#!/usr/bin/env node
/**
 * Comprehensive Flash Tool Test Suite
 * 
 * Tests the flash tool against real hardware on headless servers.
 * Uses Xvfb for virtual display support if needed.
 * 
 * Environment Variables:
 *   TEST_PORT - Serial port to test with (default: auto-detect)
 *   TEST_BOARD - Board type to test (default: auto-detect)
 *   TEST_FIRMWARE_DIR - Directory containing test firmware
 *   DISPLAY - X display (default: :99 with Xvfb)
 * 
 * Usage:
 *   # Setup virtual display
 *   export DISPLAY=:99
 *   Xvfb :99 -screen 0 1024x768x24 &
 *   
 *   # Run tests
 *   node test/flash-test-suite.js
 * 
 * Or use the convenience script:
 *   ./test/run-headless-tests.sh
 */

const { spawn, exec } = require('child_process');
const { promisify } = require('util');
const fs = require('fs').promises;
const path = require('path');

const execAsync = promisify(exec);

// Test configuration
const config = {
  port: process.env.TEST_PORT || null,
  board: process.env.TEST_BOARD || null,
  firmwareDir: process.env.TEST_FIRMWARE_DIR || './test-firmware',
  timeout: parseInt(process.env.TEST_TIMEOUT || '60000', 10), // 60 seconds default
  xvfbDisplay: process.env.DISPLAY || ':99',
  xvfbResolution: process.env.XVFB_RESOLUTION || '1024x768x24'
};

// Test results
const results = {
  total: 0,
  passed: 0,
  failed: 0,
  skipped: 0,
  tests: []
};

function log(message, type = 'info') {
  const timestamp = new Date().toISOString();
  const icons = {
    info: 'ℹ️',
    success: '✅',
    error: '❌',
    warning: '⚠️',
    test: '🧪'
  };
  
  const prefix = icons[type] || 'ℹ️';
  console.log(`[${timestamp}] ${prefix} ${message}`);
}

async function checkDependencies() {
  log('Checking dependencies...', 'info');
  
  const checks = [
    { cmd: 'node', flag: '--version', name: 'Node.js' },
    { cmd: 'electron', flag: '--version', name: 'Electron' },
    { cmd: 'esptool.py', flag: '--version', name: 'esptool' },
    { cmd: 'Xvfb', flag: '-help', name: 'Xvfb (for headless display)' }
  ];
  
  for (const check of checks) {
    try {
      await execAsync(`${check.cmd} ${check.flag}`, { timeout: 5000 });
      log(`${check.name} found`, 'success');
    } catch (error) {
      log(`${check.name} not found: ${error.message}`, 'warning');
      if (check.name === 'esptool') {
        log('  Install with: pip install esptool', 'info');
      } else if (check.name.includes('Xvfb')) {
        log('  Install with: sudo apt-get install xvfb', 'info');
        log('  Or set DISPLAY to existing X server', 'info');
      }
    }
  }
}

async function setupVirtualDisplay() {
  if (process.env.DISPLAY) {
    log(`Using existing DISPLAY: ${process.env.DISPLAY}`, 'info');
    return null;
  }
  
  log(`Setting up virtual display ${config.xvfbDisplay}...`, 'info');
  
  return new Promise((resolve, reject) => {
    // Check if Xvfb is already running on this display
    exec(`xdpyinfo -display ${config.xvfbDisplay}`, (error) => {
      if (!error) {
        log(`Display ${config.xvfbDisplay} already exists`, 'info');
        resolve(null);
        return;
      }
      
      // Start Xvfb
      const xvfb = spawn('Xvfb', [
        config.xvfbDisplay,
        '-screen', '0', config.xvfbResolution,
        '-ac', // Disable access control
        '+extension', 'RANDR'
      ], {
        stdio: 'ignore',
        detached: true
      });
      
      xvfb.unref();
      
      // Wait a moment for Xvfb to start
      setTimeout(() => {
        // Verify it's running
        exec(`xdpyinfo -display ${config.xvfbDisplay}`, (verifyError) => {
          if (verifyError) {
            reject(new Error(`Failed to start Xvfb: ${verifyError.message}`));
          } else {
            log(`Xvfb started successfully on ${config.xvfbDisplay}`, 'success');
            process.env.DISPLAY = config.xvfbDisplay;
            resolve(xvfb);
          }
        });
      }, 2000);
    });
  });
}

async function findTestFirmware() {
  log(`Looking for test firmware in ${config.firmwareDir}...`, 'info');
  
  try {
    const files = await fs.readdir(config.firmwareDir);
    const binFiles = files.filter(f => f.endsWith('.bin'));
    
    if (binFiles.length === 0) {
      throw new Error(`No .bin files found in ${config.firmwareDir}`);
    }
    
    log(`Found ${binFiles.length} firmware file(s)`, 'success');
    binFiles.forEach(file => {
      log(`  - ${file}`, 'info');
    });
    
    return config.firmwareDir;
  } catch (error) {
    log(`Test firmware directory not found: ${error.message}`, 'warning');
    log(`Set TEST_FIRMWARE_DIR environment variable to specify firmware location`, 'info');
    return null;
  }
}

async function listSerialPorts() {
  log('Scanning for serial ports...', 'info');
  
  try {
    // Use serialport library
    const { SerialPort } = require('serialport');
    const ports = await SerialPort.list();
    
    const esp32Ports = ports.filter(port => {
      const vendorId = (port.vendorId || '').toLowerCase();
      const esp32VendorIds = ['10c4', '1a86', '303a', '2e8a'];
      return esp32VendorIds.some(vid => vendorId.includes(vid));
    });
    
    log(`Found ${ports.length} serial port(s)`, 'info');
    if (esp32Ports.length > 0) {
      log(`Found ${esp32Ports.length} potential ESP32 device(s):`, 'success');
      esp32Ports.forEach(port => {
        log(`  ${port.path} - ${port.manufacturer || 'Unknown'} (VID:${port.vendorId})`, 'info');
      });
    }
    
    return ports;
  } catch (error) {
    log(`Error listing ports: ${error.message}`, 'error');
    throw error;
  }
}

async function detectChipType(port) {
  log(`Detecting chip type on ${port}...`, 'info');
  
  return new Promise((resolve, reject) => {
    const esptool = spawn('esptool.py', ['--port', port, 'flash_id'], {
      stdio: ['ignore', 'pipe', 'pipe']
    });
    
    let output = '';
    let errorOutput = '';
    
    esptool.stdout.on('data', (data) => {
      output += data.toString();
    });
    
    esptool.stderr.on('data', (data) => {
      errorOutput += data.toString();
    });
    
    esptool.on('close', (code) => {
      const fullOutput = (output + errorOutput).toLowerCase();
      
      let chipType = null;
      const chipPatterns = [
        { pattern: 'esp32c3', type: 'esp32c3' },
        { pattern: 'esp32c6', type: 'esp32c6' },
        { pattern: 'esp32s3', type: 'esp32s3' },
        { pattern: 'esp32s2', type: 'esp32s2' },
        { pattern: 'esp32[^-]', type: 'esp32' }
      ];
      
      for (const { pattern, type } of chipPatterns) {
        if (new RegExp(pattern).test(fullOutput)) {
          chipType = type;
          break;
        }
      }
      
      if (chipType) {
        log(`Detected chip: ${chipType}`, 'success');
        resolve(chipType);
      } else {
        log(`Could not detect chip type`, 'warning');
        log(`Output: ${fullOutput.substring(0, 200)}`, 'info');
        resolve(null);
      }
    });
    
    esptool.on('error', (err) => {
      log(`esptool error: ${err.message}`, 'error');
      reject(err);
    });
    
    // Timeout
    setTimeout(() => {
      esptool.kill();
      reject(new Error('Chip detection timeout'));
    }, 10000);
  });
}

async function runTest(testName, testFn) {
  results.total++;
  log(`Running test: ${testName}`, 'test');
  
  const startTime = Date.now();
  
  try {
    await Promise.race([
      testFn(),
      new Promise((_, reject) => 
        setTimeout(() => reject(new Error('Test timeout')), config.timeout)
      )
    ]);
    
    const duration = Date.now() - startTime;
    results.passed++;
    results.tests.push({ name: testName, status: 'passed', duration });
    log(`Test passed: ${testName} (${duration}ms)`, 'success');
    return true;
  } catch (error) {
    const duration = Date.now() - startTime;
    results.failed++;
    results.tests.push({ name: testName, status: 'failed', duration, error: error.message });
    log(`Test failed: ${testName} - ${error.message}`, 'error');
    return false;
  }
}

// Test cases
async function testSerialPortDetection() {
  const ports = await listSerialPorts();
  if (ports.length === 0) {
    throw new Error('No serial ports found');
  }
  
  if (!config.port) {
    // Auto-select first ESP32-looking port
    const esp32Ports = ports.filter(p => {
      const vid = (p.vendorId || '').toLowerCase();
      return ['10c4', '1a86', '303a', '2e8a'].some(v => vid.includes(v));
    });
    
    if (esp32Ports.length > 0) {
      config.port = esp32Ports[0].path;
      log(`Auto-selected port: ${config.port}`, 'info');
    } else {
      throw new Error('No ESP32-like devices found');
    }
  }
}

async function testChipDetection() {
  if (!config.port) {
    throw new Error('No port specified for chip detection');
  }
  
  const chipType = await detectChipType(config.port);
  if (!chipType) {
    throw new Error('Could not detect chip type');
  }
  
  config.detectedChip = chipType;
}

async function testFirmwareFiles() {
  const firmwareDir = await findTestFirmware();
  if (!firmwareDir) {
    throw new Error('No test firmware found');
  }
  
  const requiredFiles = ['firmware.bin'];
  for (const file of requiredFiles) {
    const filePath = path.join(firmwareDir, file);
    try {
      await fs.access(filePath);
    } catch (error) {
      throw new Error(`Required firmware file not found: ${file}`);
    }
  }
  
  config.firmwarePath = firmwareDir;
}

async function testElectronAppLoad() {
  // Test that Electron can start (with virtual display)
  return new Promise((resolve, reject) => {
    const electron = spawn('electron', ['--version'], {
      env: { ...process.env, DISPLAY: config.xvfbDisplay },
      stdio: 'pipe'
    });
    
    let output = '';
    electron.stdout.on('data', (data) => {
      output += data.toString();
    });
    
    electron.on('close', (code) => {
      if (code === 0) {
        log(`Electron version: ${output.trim()}`, 'success');
        resolve();
      } else {
        reject(new Error(`Electron failed with code ${code}`));
      }
    });
    
    electron.on('error', (err) => {
      reject(err);
    });
  });
}

// Main test runner
async function runAllTests() {
  log('Starting Flash Tool Test Suite', 'test');
  log(`Configuration:`, 'info');
  log(`  Display: ${config.xvfbDisplay}`, 'info');
  log(`  Firmware Dir: ${config.firmwareDir}`, 'info');
  log(`  Timeout: ${config.timeout}ms`, 'info');
  
  try {
    // Setup
    await checkDependencies();
    const xvfb = await setupVirtualDisplay();
    
    // Run tests
    await runTest('Serial Port Detection', testSerialPortDetection);
    await runTest('Chip Detection', testChipDetection);
    await runTest('Firmware Files Check', testFirmwareFiles);
    await runTest('Electron App Load', testElectronAppLoad);
    
    // Summary
    log('\n=== Test Summary ===', 'info');
    log(`Total: ${results.total}`, 'info');
    log(`Passed: ${results.passed}`, 'success');
    log(`Failed: ${results.failed}`, results.failed > 0 ? 'error' : 'success');
    log(`Skipped: ${results.skipped}`, 'info');
    
    // Failed tests details
    if (results.failed > 0) {
      log('\nFailed Tests:', 'error');
      results.tests
        .filter(t => t.status === 'failed')
        .forEach(t => {
          log(`  - ${t.name}: ${t.error}`, 'error');
        });
    }
    
    process.exit(results.failed > 0 ? 1 : 0);
  } catch (error) {
    log(`Fatal error: ${error.message}`, 'error');
    console.error(error);
    process.exit(1);
  }
}

// Run if executed directly
if (require.main === module) {
  runAllTests();
}

module.exports = { runAllTests, runTest, config };

