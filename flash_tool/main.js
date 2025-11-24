const { app, BrowserWindow, ipcMain, dialog } = require('electron');
const path = require('path');
const { spawn } = require('child_process');
const fs = require('fs').promises;
const { SerialPort } = require('serialport');
const axios = require('axios');
const extractZip = require('extract-zip');

let mainWindow;

// Serial monitor state
let monitorPort = null;
let monitorPortPath = null;

// Board configurations matching platformio.ini
const BOARD_CONFIGS = {
  'esp32-c3-supermini': {
    name: 'ESP32-C3 SuperMini',
    chip: 'esp32c3',
    flashAddresses: {
      bootloader: '0x0',
      partitions: '0x8000',
      firmware: '0x10000',
      spiffs: '0x290000'
    }
  },
  'nuclearcounter': {
    name: 'NuclearCounter (ESP32-C3)',
    chip: 'esp32c3',
    flashAddresses: {
      bootloader: '0x0',
      partitions: '0x8000',
      firmware: '0x10000',
      spiffs: '0x290000'
    }
  },
  'esp32-c3': {
    name: 'ESP32-C3 Dev Module',
    chip: 'esp32c3',
    flashAddresses: {
      bootloader: '0x0',
      partitions: '0x8000',
      firmware: '0x10000',
      spiffs: '0x290000'
    }
  },
  'esp32-c6': {
    name: 'ESP32-C6 Dev Module (WiFi 6)',
    chip: 'esp32c6',
    flashAddresses: {
      bootloader: '0x0',
      partitions: '0x8000',
      firmware: '0x10000',
      spiffs: '0x290000'
    }
  },
  'esp32dev': {
    name: 'ESP32 Dev Module',
    chip: 'esp32',
    flashAddresses: {
      bootloader: '0x1000',
      partitions: '0x8000',
      firmware: '0x10000',
      spiffs: '0x290000'
    }
  },
  'esp32-s3': {
    name: 'ESP32-S3 Dev Module',
    chip: 'esp32s3',
    flashAddresses: {
      bootloader: '0x0',
      partitions: '0x8000',
      firmware: '0x10000',
      spiffs: '0x290000'
    }
  },
  'esp32-s3-touch': {
    name: 'ESP32-S3 Touch LCD (no PSRAM)',
    chip: 'esp32s3',
    flashAddresses: {
      bootloader: '0x0',
      partitions: '0x8000',
      firmware: '0x10000',
      spiffs: '0x290000'
    }
  },
  'esp32-s2': {
    name: 'ESP32-S2 Dev Module',
    chip: 'esp32s2',
    flashAddresses: {
      bootloader: '0x1000',
      partitions: '0x8000',
      firmware: '0x10000',
      spiffs: '0x290000'
    }
  },
  'jc2432w328c': {
    name: 'JC2432W328C (ESP32 with LCD)',
    chip: 'esp32',
    flashAddresses: {
      bootloader: '0x1000',
      partitions: '0x8000',
      firmware: '0x10000',
      spiffs: '0x290000'
    }
  }
};

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, 'preload.js')
    },
    title: 'StarForge Flash Tool',
    resizable: true,
    autoHideMenuBar: true
  });

  mainWindow.loadFile('index.html');

  // Open DevTools in development
  if (process.env.NODE_ENV === 'development') {
    mainWindow.webContents.openDevTools();
  }
}

app.whenReady().then(() => {
  createWindow();

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

// IPC Handlers

// Get available serial ports
ipcMain.handle('get-serial-ports', async () => {
  try {
    const ports = await SerialPort.list();
    return ports.map(port => ({
      path: port.path,
      manufacturer: port.manufacturer || 'Unknown',
      serialNumber: port.serialNumber || '',
      productId: port.productId || '',
      vendorId: port.vendorId || '',
      // Check if this looks like an ESP32 device
      isESP32: isLikelyESP32Port(port)
    }));
  } catch (error) {
    console.error('Error listing serial ports:', error);
    return [];
  }
});

// Start serial monitor (read-only)
ipcMain.handle('start-serial-monitor', async (event, options) => {
  const { port, baudRate = 115200 } = options || {};

  if (!port) {
    throw new Error('No serial port specified for monitor');
  }

  // If we're already monitoring this port, do nothing
  if (monitorPort && monitorPortPath === port && monitorPort.readable) {
    return { success: true };
  }

  // Close any existing monitor port first
  if (monitorPort) {
    try {
      monitorPort.close();
    } catch (e) {
      // Ignore errors on close
    }
    monitorPort = null;
    monitorPortPath = null;
  }

  return new Promise((resolve, reject) => {
    const sp = new SerialPort({
      path: port,
      baudRate,
      autoOpen: true
    });

    sp.on('open', () => {
      monitorPort = sp;
      monitorPortPath = port;
      if (mainWindow && !mainWindow.isDestroyed()) {
        mainWindow.webContents.send('serial-monitor-status', {
          type: 'open',
          port,
          baudRate
        });
      }
      resolve({ success: true });
    });

    sp.on('data', (data) => {
      if (mainWindow && !mainWindow.isDestroyed()) {
        mainWindow.webContents.send('serial-monitor-data', data.toString());
      }
    });

    sp.on('error', (err) => {
      if (mainWindow && !mainWindow.isDestroyed()) {
        mainWindow.webContents.send('serial-monitor-status', {
          type: 'error',
          port,
          message: err.message
        });
      }
      // Only reject if this happened before open
      if (!monitorPort || monitorPort !== sp) {
        reject(new Error(`Serial monitor error: ${err.message}`));
      }
    });

    sp.on('close', () => {
      if (mainWindow && !mainWindow.isDestroyed()) {
        mainWindow.webContents.send('serial-monitor-status', {
          type: 'close',
          port
        });
      }
      if (monitorPort === sp) {
        monitorPort = null;
        monitorPortPath = null;
      }
    });
  });
});

// Stop serial monitor
ipcMain.handle('stop-serial-monitor', async () => {
  if (!monitorPort) {
    return { success: true };
  }

  return new Promise((resolve) => {
    try {
      const port = monitorPortPath;
      monitorPort.close((/*err*/) => {
        if (mainWindow && !mainWindow.isDestroyed()) {
          mainWindow.webContents.send('serial-monitor-status', {
            type: 'close',
            port
          });
        }
        monitorPort = null;
        monitorPortPath = null;
        resolve({ success: true });
      });
    } catch (e) {
      monitorPort = null;
      monitorPortPath = null;
      resolve({ success: true });
    }
  });
});

// Helper to identify likely ESP32 ports
function isLikelyESP32Port(port) {
  // Common USB vendor IDs for ESP32 devices
  const esp32VendorIds = [
    '10c4', // Silicon Labs (CP210x) - most common
    '1a86', // QinHeng Electronics (CH340)
    '0403', // FTDI
    '067b', // Prolific
    '303a', // Espressif (native USB on ESP32-S2/S3/C3)
  ];
  
  // Common manufacturer names
  const esp32Manufacturers = [
    'Silicon Labs',
    'Silicon Laboratories',
    'Espressif', // Native USB
    'Expressif', // Sometimes misspelled
    'QinHeng',
    'FTDI',
    'Prolific',
  ];
  
  const vendorId = (port.vendorId || '').toLowerCase();
  const manufacturer = (port.manufacturer || '').toLowerCase();
  
  return esp32VendorIds.includes(vendorId) || 
         esp32Manufacturers.some(m => manufacturer.includes(m.toLowerCase()));
}

// Detect board type by querying the chip
ipcMain.handle('detect-board', async (event, port) => {
  return new Promise((resolve, reject) => {
    let esptoolCmd;
    try {
      esptoolCmd = findEsptool();
    } catch (error) {
      reject(error);
      return;
    }
    
    // Run esptool flash_id to detect chip type
    const args = ['--port', port, 'flash_id'];
    const esptool = spawn(esptoolCmd, args);
    
    let output = '';
    
    esptool.stdout.on('data', (data) => {
      output += data.toString();
    });
    
    esptool.stderr.on('data', (data) => {
      output += data.toString();
    });
    
    esptool.on('close', (code) => {
      if (code === 0 || output.includes('Detecting chip type')) {
        // Parse chip type from output
        const chipType = parseChipType(output);
        resolve({ 
          success: true, 
          chipType,
          suggestedBoard: suggestBoardFromChip(chipType),
          output 
        });
      } else {
        reject(new Error('Failed to detect chip'));
      }
    });
    
    esptool.on('error', (err) => {
      reject(new Error(`Failed to run esptool: ${err.message}`));
    });
  });
});

// Parse chip type from esptool output
function parseChipType(output) {
  const chipPatterns = [
    /Detecting chip type\.\.\. (ESP32[^\s\n]*)/i,
    /Chip is (ESP32[^\s\n]*)/i,
    /Detecting chip type\.\.\. (.+)/i,
  ];
  
  for (const pattern of chipPatterns) {
    const match = output.match(pattern);
    if (match) {
      return match[1].trim();
    }
  }
  
  return 'Unknown';
}

// Suggest board configuration based on detected chip
function suggestBoardFromChip(chipType) {
  const chipLower = chipType.toLowerCase();
  
  if (chipLower.includes('esp32-c3')) {
    return 'esp32-c3-supermini'; // Most common C3 board (NuclearCounter is C3-based as well)
  } else if (chipLower.includes('esp32-s3')) {
    return 'esp32-s3';
  } else if (chipLower.includes('esp32-s2')) {
    return 'esp32-s2';
  } else if (chipLower.includes('esp32')) {
    return 'esp32dev'; // Original ESP32
  }
  
  return null;
}

// Get board configurations
ipcMain.handle('get-board-configs', async () => {
  return BOARD_CONFIGS;
});

// Fetch releases from GitHub
ipcMain.handle('fetch-github-releases', async () => {
  try {
    // Fetch all releases (including pre-releases) and the latest stable release
    const [allReleasesResponse, latestResponse] = await Promise.all([
      axios.get(
        'https://api.github.com/repos/RaceFPV/StarForgeOS/releases',
        {
          headers: {
            'Accept': 'application/vnd.github.v3+json',
            'User-Agent': 'StarForge-Flasher'
          }
        }
      ),
      axios.get(
        'https://api.github.com/repos/RaceFPV/StarForgeOS/releases/latest',
        {
          headers: {
            'Accept': 'application/vnd.github.v3+json',
            'User-Agent': 'StarForge-Flasher'
          }
        }
      ).catch(() => ({ data: null })) // If latest fails, continue without it
    ]);
    
    if (!allReleasesResponse.data || allReleasesResponse.data.length === 0) {
      throw new Error('No releases found');
    }
    
    // Get the latest stable release tag (excludes pre-releases)
    const latestTag = latestResponse.data ? latestResponse.data.tag_name : null;
    
    // Return all releases with latest indicator
    return {
      releases: allReleasesResponse.data.map(release => ({
        tag: release.tag_name,
        name: release.name || release.tag_name,
        publishedAt: release.published_at,
        isPrerelease: release.prerelease,
        assets: release.assets.map(asset => ({
          name: asset.name,
          url: asset.browser_download_url,
          size: asset.size
        }))
      })),
      latestTag: latestTag // The actual latest stable release tag
    };
  } catch (error) {
    console.error('Error fetching releases:', error);
    if (error.response) {
      throw new Error(`GitHub API error: ${error.response.status} - ${error.response.statusText}`);
    }
    throw new Error('Failed to fetch releases from GitHub. Make sure releases are published.');
  }
});

// Download firmware
ipcMain.handle('download-firmware', async (event, url, boardType) => {
  try {
    const cacheDir = path.join(app.getPath('userData'), 'firmware-cache');
    await fs.mkdir(cacheDir, { recursive: true });
    
    // Include URL hash in cache key to ensure different releases get different cache entries
    // This prevents stale firmware from being used when a new release is downloaded
    const crypto = require('crypto');
    const urlHash = crypto.createHash('md5').update(url).digest('hex').substring(0, 8);
    const zipPath = path.join(cacheDir, `${boardType}-${urlHash}.zip`);
    const extractPath = path.join(cacheDir, `${boardType}-${urlHash}`);
    
    // Clear extract directory if it exists to ensure fresh extraction
    // This prevents stale files from previous extractions
    try {
      const stats = await fs.stat(extractPath).catch(() => null);
      if (stats && stats.isDirectory()) {
        // Remove directory (fs.rm is available in Node 14.14.0+, which Electron uses)
        await fs.rm(extractPath, { recursive: true, force: true });
        event.sender.send('flash-progress', `Cleared old cached firmware...\n`);
      }
    } catch (err) {
      // Ignore errors - directory might not exist or be locked
      // Continue with download anyway
    }
    
    // Download
    event.sender.send('flash-progress', `Downloading firmware from: ${url}\n`);
    const response = await axios({
      method: 'get',
      url: url,
      responseType: 'stream',
      onDownloadProgress: (progressEvent) => {
        const percentCompleted = Math.round((progressEvent.loaded * 100) / progressEvent.total);
        event.sender.send('download-progress', percentCompleted);
      }
    });
    
    const writer = require('fs').createWriteStream(zipPath);
    response.data.pipe(writer);
    
    await new Promise((resolve, reject) => {
      writer.on('finish', resolve);
      writer.on('error', reject);
    });
    
    // Extract to fresh directory
    event.sender.send('flash-progress', `Extracting firmware...\n`);
    await extractZip(zipPath, { dir: extractPath });
    event.sender.send('flash-progress', `✓ Firmware extracted to: ${extractPath}\n`);
    
    return {
      path: extractPath,
      files: await fs.readdir(extractPath)
    };
  } catch (error) {
    console.error('Error downloading firmware:', error);
    throw error;
  }
});

// Find esptool command
function findEsptool() {
  // Determine platform-specific binary name
  let binaryName;
  if (process.platform === 'darwin') {
    binaryName = 'esptool-macos';
  } else if (process.platform === 'win32') {
    binaryName = 'esptool-win64.exe';
  } else {
    binaryName = 'esptool-linux';
  }
  
  // Try bundled version first (production)
  if (app.isPackaged) {
    const bundledPath = path.join(process.resourcesPath, 'resources', 'bin', binaryName);
    if (require('fs').existsSync(bundledPath)) {
      return bundledPath;
    }
  }
  
  // Try development version (running from source)
  const devPath = path.join(__dirname, 'resources', 'bin', binaryName);
  if (require('fs').existsSync(devPath)) {
    return devPath;
  }
  
  // Fallback: try system esptool (for development without downloaded binaries)
  const commands = process.platform === 'win32' 
    ? ['esptool.exe', 'esptool.py', 'esptool']
    : ['esptool.py', 'esptool'];
  
  for (const cmd of commands) {
    try {
      require('child_process').execSync(`${cmd} version`, { stdio: 'ignore' });
      return cmd;
    } catch (e) {
      // Command not found, try next
    }
  }
  
  throw new Error(`esptool not found. Binary should be at: ${devPath}\n\nRun: ./download-esptool.sh`);
}

// Parse partition table to find SPIFFS address
function findSPIFFSInPartitionTable(partitionData) {
  // Partition table entry is 32 bytes:
  // - 16 bytes: name (null-terminated)
  // - 2 bytes: type
  // - 1 byte: subtype
  // - 4 bytes: offset (little-endian)
  // - 4 bytes: size
  // - remaining: flags/reserved
  
  // Partition table typically starts at offset 0x1000 (4096) in the binary
  // But the .bin file might be just the table itself, starting at 0
  let tableStart = 0;
  
  // Try to find partition table signature or start scanning from beginning
  // PlatformIO partition tables can have different formats
  let offset = tableStart;
  
  while (offset + 32 <= partitionData.length) {
    // Read name (first 16 bytes, null-terminated)
    const nameBytes = partitionData.slice(offset, offset + 16);
    const name = nameBytes.toString('utf8').split('\0')[0].trim();
    
    // Check for end marker (all zeros or invalid name)
    if (!name || name.length === 0 || name.charCodeAt(0) === 0 || name.charCodeAt(0) === 0xFF) {
      // Skip empty entries, but continue scanning
      offset += 32;
      continue;
    }
    
    // Read type (2 bytes at offset 16)
    const type = partitionData.readUInt16LE(offset + 16);
    
    // Read subtype (byte at offset 18)
    const subtype = partitionData.readUInt8(offset + 18);
    
    // Read offset (4 bytes little-endian at offset 20)
    const entryOffset = partitionData.readUInt32LE(offset + 20);
    
    // Read size (4 bytes little-endian at offset 24)
    const entrySize = partitionData.readUInt32LE(offset + 24);
    
    // Check if this is SPIFFS partition
    // Subtype 0x82 = SPIFFS (data), type 0x01 = app, type 0x82 = data
    // Also check by name (case-insensitive)
    const nameLower = name.toLowerCase();
    if (subtype === 0x82 || nameLower === 'spiffs' || nameLower.includes('spiffs')) {
      return `0x${entryOffset.toString(16)}`;
    }
    
    offset += 32;
    
    // Safety: don't scan more than 16 entries (512 bytes)
    if (offset > 512) {
      break;
    }
  }
  
  return null;
}

// Generate SPIFFS partition with custom config
async function generateCustomSPIFFS(customConfig) {
  const tempDir = app.getPath('temp');
  const configJsonPath = path.join(tempDir, 'sfos_custom_config.json');
  const spiffsImagePath = path.join(tempDir, 'custom_spiffs.bin');
  
  // Write config.json
  await fs.writeFile(configJsonPath, JSON.stringify(customConfig, null, 2));
  
  // Run Python script to generate SPIFFS
  const scriptPath = path.join(__dirname, 'resources', 'scripts', 'generate_spiffs.py');
  
  return new Promise((resolve, reject) => {
    const python = spawn('python3', [scriptPath, configJsonPath, spiffsImagePath]);
    
    let output = '';
    
    python.stdout.on('data', (data) => {
      output += data.toString();
      console.log(data.toString());
    });
    
    python.stderr.on('data', (data) => {
      output += data.toString();
      console.error(data.toString());
    });
    
    python.on('close', (code) => {
      if (code === 0) {
        resolve(spiffsImagePath);
      } else {
        reject(new Error(`SPIFFS generation failed: ${output}`));
      }
    });
    
    python.on('error', (err) => {
      // Try 'python' instead of 'python3'
      if (err.code === 'ENOENT') {
        const pythonAlt = spawn('python', [scriptPath, configJsonPath, spiffsImagePath]);
        
        pythonAlt.stdout.on('data', (data) => console.log(data.toString()));
        pythonAlt.stderr.on('data', (data) => console.error(data.toString()));
        
        pythonAlt.on('close', (code) => {
          if (code === 0) {
            resolve(spiffsImagePath);
          } else {
            reject(new Error('SPIFFS generation failed'));
          }
        });
        
        pythonAlt.on('error', (err2) => {
          reject(new Error(`Python not found: ${err2.message}`));
        });
      } else {
        reject(err);
      }
    });
  });
}

// Check if a path is a PlatformIO project
async function isPlatformIOProject(folderPath) {
  const platformioIni = path.join(folderPath, 'platformio.ini');
  try {
    await fs.access(platformioIni);
    return true;
  } catch {
    return false;
  }
}

// Find PlatformIO CLI command
function findPlatformIO() {
  // Determine platform-specific binary name
  let binaryName;
  if (process.platform === 'darwin') {
    binaryName = 'platformio-macos';
  } else if (process.platform === 'win32') {
    binaryName = 'platformio-win64.bat';  // Use .bat on Windows
  } else {
    binaryName = 'platformio-linux';
  }
  
  // Try bundled version first (production)
  if (app.isPackaged) {
    const bundledPath = path.join(process.resourcesPath, 'resources', 'bin', binaryName);
    if (require('fs').existsSync(bundledPath)) {
      return bundledPath;
    }
  }
  
  // Try development version (running from source)
  const devPath = path.join(__dirname, 'resources', 'bin', binaryName);
  if (require('fs').existsSync(devPath)) {
    return devPath;
  }
  
  // Fallback: try system PlatformIO (for development without bundled binaries)
  const commands = process.platform === 'win32' 
    ? ['pio', 'platformio', 'pio.exe', 'platformio.exe']
    : ['pio', 'platformio'];
  
  for (const cmd of commands) {
    try {
      require('child_process').execSync(`${cmd} --version`, { stdio: 'ignore' });
      return cmd;
    } catch (e) {
      // Command not found, try next
    }
  }
  
  return null;
}

// Flash firmware using esptool.py or PlatformIO
ipcMain.handle('flash-firmware', async (event, options) => {
  const { port, boardType, firmwarePath, baudRate = 921600, customConfig = null } = options;
  const config = BOARD_CONFIGS[boardType];
  
  if (!config) {
    throw new Error('Invalid board type');
  }
  
  // Check if this is a PlatformIO project (source code)
  const isPIOProject = await isPlatformIOProject(firmwarePath);
  
  if (isPIOProject) {
    // Use PlatformIO to build and upload
    return flashWithPlatformIO(event, firmwarePath, boardType, port, customConfig);
  }
  
  // Otherwise, use esptool with pre-built binaries
  return new Promise(async (resolve, reject) => {
    // Debug: Show what we're flashing
    event.sender.send('flash-progress', `\n=== Flashing pre-built firmware ===\n`);
    event.sender.send('flash-progress', `Board Type: ${boardType}\n`);
    event.sender.send('flash-progress', `Firmware Path: ${firmwarePath}\n`);
    event.sender.send('flash-progress', `Port: ${port}\n\n`);
    
    let esptoolCmd;
    try {
      esptoolCmd = findEsptool();
    } catch (error) {
      reject(error);
      return;
    }
    
    // Build esptool.py command
    const args = [
      '--chip', config.chip,
      '--port', port,
      '--baud', baudRate.toString(),
      '--before', 'default_reset',
      '--after', 'hard_reset',
      'write_flash'
    ];
    
    // Note: --flash_mode and --flash_size are not needed for write_flash
    // esptool auto-detects these from the chip and partition table
    // Adding them causes errors in some esptool versions
    
    // Add flash addresses and files
    const files = {
      bootloader: path.join(firmwarePath, 'bootloader.bin'),
      partitions: path.join(firmwarePath, 'partitions.bin'),
      firmware: path.join(firmwarePath, 'firmware.bin'),
      spiffs: path.join(firmwarePath, 'spiffs.bin')
    };
    
    // Try to read SPIFFS address from partitions.bin (more accurate than hardcoded)
    // This ensures we use the correct address from the actual partition table
    let spiffsAddress = config.flashAddresses.spiffs;
    if (require('fs').existsSync(files.partitions)) {
      try {
        // Read partition table to find SPIFFS address
        const partitionData = require('fs').readFileSync(files.partitions);
        const spiffsAddr = findSPIFFSInPartitionTable(partitionData);
        if (spiffsAddr) {
          spiffsAddress = spiffsAddr;
          event.sender.send('flash-progress', `✓ Read SPIFFS address from partition table: ${spiffsAddress}\n`);
        } else {
          // Debug: Try to dump partition table entries for troubleshooting
          event.sender.send('flash-progress', `⚠ Could not find SPIFFS in partition table (${partitionData.length} bytes)\n`);
          // Try to list all partitions found for debugging
          let offset = 0;
          const foundPartitions = [];
          while (offset + 32 <= partitionData.length && foundPartitions.length < 10) {
            const nameBytes = partitionData.slice(offset, offset + 16);
            const name = nameBytes.toString('utf8').split('\0')[0].trim();
            if (name && name.length > 0 && name.charCodeAt(0) !== 0 && name.charCodeAt(0) !== 0xFF) {
              const entryOffset = partitionData.readUInt32LE(offset + 20);
              const subtype = partitionData.readUInt8(offset + 18);
              foundPartitions.push(`${name}@0x${entryOffset.toString(16)} (subtype:0x${subtype.toString(16)})`);
            }
            offset += 32;
            if (offset > 512) break;
          }
          if (foundPartitions.length > 0) {
            event.sender.send('flash-progress', `   Found partitions: ${foundPartitions.join(', ')}\n`);
          }
          event.sender.send('flash-progress', `   Using default SPIFFS address: ${spiffsAddress}\n`);
        }
      } catch (error) {
        event.sender.send('flash-progress', `⚠ Could not read partition table: ${error.message}, using default: ${spiffsAddress}\n`);
      }
    }
    
    // Check which files exist
    // IMPORTANT: Build file list in specific order to ensure SPIFFS is written last
    // This prevents any potential overwrite issues
    const fileOrder = ['bootloader', 'partitions', 'firmware', 'spiffs'];
    const existingFiles = [];
    const fileArgs = [];  // Separate array to control order
    
    // First pass: collect all files except SPIFFS
    fileOrder.forEach(key => {
      if (key === 'spiffs') return;  // Skip SPIFFS for now
      if (require('fs').existsSync(files[key])) {
        existingFiles.push(key);
        fileArgs.push(config.flashAddresses[key], files[key]);
        
      }
    });
    
    // SPIFFS will be uploaded separately using PlatformIO's uploadfs command
    // This ensures PlatformIO handles the correct SPIFFS address from the partition table
    let spiffsFile = null;
    if (require('fs').existsSync(files.spiffs)) {
      existingFiles.push('spiffs');
      spiffsFile = files.spiffs;
      
      // Check SPIFFS file size and warn if suspiciously small
      const stats = require('fs').statSync(files.spiffs);
      const sizeKB = stats.size / 1024;
      event.sender.send('flash-progress', `Found spiffs.bin (${sizeKB.toFixed(1)} KB)\n`);
      
      // Verify SPIFFS file is valid
      if (stats.size < 1024) {
        event.sender.send('flash-progress', `⚠️ WARNING: spiffs.bin is very small (${stats.size} bytes). It may be empty or incomplete.\n`);
      } else if (stats.size > 10 * 1024 * 1024) {
        event.sender.send('flash-progress', `⚠️ WARNING: spiffs.bin is unusually large (${sizeKB.toFixed(1)} KB). This may indicate corruption.\n`);
      } else {
        // Try to read first few bytes to verify it's not all zeros or corrupted
        const fs = require('fs');
        const buffer = Buffer.alloc(256);
        const fd = fs.openSync(files.spiffs, 'r');
        const bytesRead = fs.readSync(fd, buffer, 0, 256, 0);
        fs.closeSync(fd);
        
        // Check if file is all zeros (common corruption pattern)
        const allZeros = buffer.slice(0, bytesRead).every(byte => byte === 0);
        if (allZeros && bytesRead > 0) {
          event.sender.send('flash-progress', `⚠️ WARNING: spiffs.bin appears to be empty (all zeros). File may be corrupted.\n`);
        } else {
          event.sender.send('flash-progress', `✓ spiffs.bin appears valid (${sizeKB.toFixed(1)} KB)\n`);
        }
      }
      
      event.sender.send('flash-progress', `✓ SPIFFS will be uploaded using PlatformIO (ensures correct address from partition table)\n`);
    } else {
      event.sender.send('flash-progress', `⚠️ WARNING: spiffs.bin not found in firmware package. SPIFFS filesystem will not be uploaded.\n`);
      event.sender.send('flash-progress', `   This means web files (index.html, etc.) will not be available.\n`);
    }
    
    // Add all file arguments to the command
    args.push(...fileArgs);
    
    // If no firmware.bin found, that's an error
    if (!existingFiles.includes('firmware')) {
      reject(new Error('No firmware files found, expected firmware.bin. If this is source code, make sure platformio.ini exists in the folder.'));
      return;
    }
    
    // Warn if SPIFFS is missing
    if (!existingFiles.includes('spiffs')) {
      event.sender.send('flash-progress', `\n⚠️ NOTE: No SPIFFS filesystem will be uploaded.\n`);
      event.sender.send('flash-progress', `   The device will work, but web interface files may be missing.\n`);
      event.sender.send('flash-progress', `   To include SPIFFS, flash from source code or ensure the release includes spiffs.bin.\n\n`);
    }
    
    // If custom config is provided, generate and add custom SPIFFS
    if (customConfig) {
      try {
        event.sender.send('flash-progress', '\n=== Generating custom config SPIFFS ===\n');
        const customSPIFFSPath = await generateCustomSPIFFS(customConfig);
        event.sender.send('flash-progress', `✓ Custom SPIFFS generated: ${customSPIFFSPath}\n`);
        
        // Remove default SPIFFS if it was added, then add custom one
        // Find and remove the SPIFFS entry from fileArgs (use the address we determined)
        const spiffsAddrIndex = fileArgs.indexOf(spiffsAddress);
        if (spiffsAddrIndex !== -1) {
          fileArgs.splice(spiffsAddrIndex, 2); // Remove address and file path
          event.sender.send('flash-progress', `Removed default SPIFFS, will use custom SPIFFS instead\n`);
        }
        
        // Add custom SPIFFS LAST to flash command (overwrites default spiffs)
        // Use the address we read from partition table (or default)
        fileArgs.push(spiffsAddress, customSPIFFSPath);
        event.sender.send('flash-progress', `✓ Custom SPIFFS will be written LAST (after firmware)\n`);
      } catch (error) {
        event.sender.send('flash-progress', `⚠ Custom SPIFFS generation failed: ${error.message}\n`);
        event.sender.send('flash-progress', 'Continuing with standard firmware (no custom pins)\n');
      }
    }
    
    // Log the exact command being run for debugging
    event.sender.send('flash-progress', `\n=== Flashing files ===\n`);
    event.sender.send('flash-progress', `Command: ${esptoolCmd} ${args.join(' ')}\n`);
    event.sender.send('flash-progress', `Files to flash:\n`);
    for (let i = 6; i < args.length; i += 2) {
      if (i + 1 < args.length) {
        const addr = args[i];
        const file = args[i + 1];
        const fileName = path.basename(file);
        event.sender.send('flash-progress', `  ${addr}: ${fileName}\n`);
      }
    }
    event.sender.send('flash-progress', `\n`);
    
    // Spawn esptool
    const esptool = spawn(esptoolCmd, args);
    
    let output = '';
    
    esptool.stdout.on('data', (data) => {
      const text = data.toString();
      output += text;
      event.sender.send('flash-progress', text);
      
      // Removed progress percentage parsing - esptool percentages are also unreliable
      // Users can see progress in the console output instead
    });
    
    esptool.stderr.on('data', (data) => {
      const text = data.toString();
      output += text;
      event.sender.send('flash-progress', text);
    });
    
    esptool.on('close', async (code) => {
      if (code === 0) {
        // If SPIFFS file exists, upload it using PlatformIO's uploadfs command
        // This ensures PlatformIO reads the correct SPIFFS address from the partition table
        if (spiffsFile && require('fs').existsSync(spiffsFile)) {
          try {
            event.sender.send('flash-progress', `\n=== Uploading SPIFFS using PlatformIO ===\n`);
            await uploadSPIFFSFromPrebuilt(event, firmwarePath, boardType, port, spiffsFile);
            event.sender.send('flash-progress', `\n✅ SPIFFS uploaded successfully using PlatformIO\n`);
          } catch (spiffsError) {
            event.sender.send('flash-progress', `\n⚠️ WARNING: SPIFFS upload failed: ${spiffsError.message}\n`);
            event.sender.send('flash-progress', `   Firmware was flashed successfully, but SPIFFS may not be available.\n`);
            // Don't fail the entire flash - firmware is already uploaded
          }
        }
        
        resolve({ success: true, output });
      } else {
        reject(new Error(`Flashing failed with code ${code}\n${output}`));
      }
    });
    
    esptool.on('error', (err) => {
      reject(new Error(`Failed to start esptool: ${err.message}`));
    });
  });
});

// Flash using PlatformIO CLI
async function flashWithPlatformIO(event, projectPath, boardType, port, customConfig) {
  return new Promise(async (resolve, reject) => {
    const pioCmd = findPlatformIO();
    
    if (!pioCmd) {
      reject(new Error('PlatformIO CLI not found. Please install PlatformIO:\n  pip install platformio\n  or\n  pip install --user platformio'));
      return;
    }
    
    // Map board type to PlatformIO environment name
    const envMap = {
      'esp32-c3-supermini': 'esp32-c3-supermini',
    'nuclearcounter': 'nuclearcounter',
      'esp32-c3': 'esp32-c3',
      'esp32-c6': 'esp32-c6',
      'esp32dev': 'esp32dev',
      'esp32-s3': 'esp32-s3',
      'esp32-s3-touch': 'esp32-s3-touch',
      'esp32-s2': 'esp32-s2',
      'jc2432w328c': 'jc2432w328c'
    };
    
    const envName = envMap[boardType];
    if (!envName) {
      reject(new Error(`Unknown board type: ${boardType}`));
      return;
    }
    
    // Build and upload firmware
    event.sender.send('flash-progress', `\n=== Building and uploading with PlatformIO ===\n`);
    event.sender.send('flash-progress', `Board Type: ${boardType}\n`);
    event.sender.send('flash-progress', `Environment: ${envName}\n`);
    event.sender.send('flash-progress', `Port: ${port}\n\n`);
    
    // Handle UNC paths on Windows (\\wsl.localhost\... paths)
    // CMD.EXE doesn't support UNC paths as working directories
    let workingDir = projectPath;
    let useShell = process.platform === 'win32';
    
    if (process.platform === 'win32' && projectPath.startsWith('\\\\')) {
      // UNC path - convert to forward slashes for better compatibility
      // Node.js can handle forward slashes on Windows
      workingDir = projectPath.replace(/\\/g, '/');
      event.sender.send('flash-progress', `Note: Converting UNC path for Windows compatibility\n`);
      // Use shell: true but with cmd /c to handle UNC paths better
      useShell = true;
    } else if (process.platform === 'win32') {
      // Regular Windows path - convert to forward slashes for Node.js
      workingDir = projectPath.replace(/\\/g, '/');
    }
    
    // PlatformIO uses PLATFORMIO_UPLOAD_PORT environment variable, not -p flag
    const env = { ...process.env };
    env.PLATFORMIO_UPLOAD_PORT = port;
    
    // Fix Unicode encoding issues on Windows and enable unbuffered output
    if (process.platform === 'win32') {
      env.PYTHONIOENCODING = 'utf-8';
      env.PYTHONUTF8 = '1';
      // Also set console codepage to UTF-8
      env.CHCP = '65001';
    }
    // Enable unbuffered Python output for real-time console updates (all platforms)
    env.PYTHONUNBUFFERED = '1';
    // Force PlatformIO to use unbuffered output
    env.PLATFORMIO_FORCE_UNBUFFERED = '1';
    
    // On Windows, try to remove specific problematic files that commonly get locked
    if (process.platform === 'win32') {
      try {
        const fs = require('fs');
        const path = require('path');
        const buildDir = path.join(workingDir, '.pio', 'build', envName);
        const problematicFiles = [
          'bootloader.bin',
          'partitions.bin',
          'firmware.bin'
        ];
        
        for (const file of problematicFiles) {
          const filePath = path.join(buildDir, file);
          try {
            if (fs.existsSync(filePath)) {
              // Try to delete with a small delay between attempts
              for (let i = 0; i < 3; i++) {
                try {
                  fs.unlinkSync(filePath);
                  break; // Success, exit retry loop
                } catch (err) {
                  if (i < 2) {
                    await new Promise(resolve => setTimeout(resolve, 500)); // Wait 500ms and retry
                  }
                }
              }
            }
          } catch (err) {
            // Ignore errors - file might not exist or be locked
          }
        }
      } catch (err) {
        // Ignore errors
      }
    }
    
    // Clean build directory first to avoid file locking issues on Windows
    // This ensures build flags are properly applied and no stale artifacts remain
    event.sender.send('flash-progress', 'Cleaning previous build artifacts...\n');
    const cleanArgs = ['run', '-e', envName, '-t', 'clean', '--verbose'];
    const cleanPio = spawn(pioCmd, cleanArgs, {
      cwd: workingDir,
      env: env,
      shell: useShell,
      stdio: ['ignore', 'pipe', 'pipe'] // Explicitly pipe stdout/stderr for real-time output
    });
    
    // Capture clean output (but don't show it unless there's an error)
    let cleanOutput = '';
    cleanPio.stdout?.on('data', (data) => {
      cleanOutput += data.toString();
    });
    cleanPio.stderr?.on('data', (data) => {
      cleanOutput += data.toString();
    });
    
    // Wait for clean to complete (don't fail if it errors - might be first build)
    await new Promise((resolve) => {
      cleanPio.on('close', (code) => {
        if (code === 0) {
          event.sender.send('flash-progress', '✓ Build directory cleaned\n');
        }
        resolve();
      });
      cleanPio.on('error', () => {
        // Ignore errors, continue anyway (might be first build)
        resolve();
      });
      // Set timeout to prevent hanging
      setTimeout(() => {
        event.sender.send('flash-progress', '⚠ Clean step timed out, continuing...\n');
        resolve();
      }, 15000); // 15 second timeout
    });
    
    // Give Windows time to release file locks after clean
    // This is especially important on Windows where file locks can persist briefly
    event.sender.send('flash-progress', 'Waiting for file locks to release...\n');
    await new Promise(resolve => setTimeout(resolve, 2000)); // 2 second delay
    
    // On all platforms, try to manually remove the build directory if it exists
    // This ensures a completely clean build and that build flags are properly applied
    // This is especially important on Windows where file locks can persist
    const fs = require('fs');
    const path = require('path');
    const buildDir = path.join(workingDir, '.pio', 'build', envName);
    try {
      if (fs.existsSync(buildDir)) {
        // Try to remove the directory (will fail silently if locked, that's OK)
        try {
          fs.rmSync(buildDir, { recursive: true, force: true, maxRetries: 3, retryDelay: 500 });
          event.sender.send('flash-progress', '✓ Build directory removed (ensures clean build with correct flags)\n');
        } catch (err) {
          // Ignore errors - files might still be locked, PlatformIO clean will handle it
          event.sender.send('flash-progress', '⚠ Some files still locked, PlatformIO will handle cleanup\n');
        }
        // Give it another moment after manual removal
        await new Promise(resolve => setTimeout(resolve, 1000));
      }
    } catch (err) {
      // Ignore errors, continue with build
    }
    
    const buildArgs = ['run', '-e', envName, '-t', 'upload', '--verbose'];
    
    // Debug: Show exact command being run
    event.sender.send('flash-progress', `Running: ${pioCmd} ${buildArgs.join(' ')}\n`);
    event.sender.send('flash-progress', `Working directory: ${workingDir}\n`);
    event.sender.send('flash-progress', `Environment: PLATFORMIO_UPLOAD_PORT=${port}\n\n`);
    
    // On Windows with batch files, we need shell: true
    // But we'll use the workingDir with forward slashes which Node.js handles
    // Use explicit stdio configuration to ensure real-time output
    const pio = spawn(pioCmd, buildArgs, {
      cwd: workingDir,
      env: env,
      shell: useShell,
      stdio: ['ignore', 'pipe', 'pipe'] // Explicitly pipe stdout/stderr for real-time output
    });
    
    let output = '';
    
    pio.stdout.on('data', (data) => {
      const text = data.toString();
      output += text;
      event.sender.send('flash-progress', text);
      // Removed progress percentage parsing - PlatformIO outputs multiple percentages
      // (build, upload, memory) that conflict, making the progress bar inaccurate
    });
    
    pio.stderr.on('data', (data) => {
      const text = data.toString();
      output += text;
      event.sender.send('flash-progress', text);
    });
    
    pio.on('close', (code) => {
      if (code === 0) {
        // Always upload SPIFFS after successful firmware upload
        // This ensures web files (index.html, style.css, app.js) are included
        if (customConfig) {
          event.sender.send('flash-progress', '\n=== Uploading SPIFFS with custom config ===\n');
          uploadSPIFFSWithConfig(event, projectPath, envName, port, customConfig, pioCmd, env, useShell, workingDir)
            .then(() => {
              resolve({ success: true, output });
            })
            .catch((error) => {
              event.sender.send('flash-progress', `⚠ SPIFFS upload failed: ${error.message}\n`);
              event.sender.send('flash-progress', 'Firmware uploaded successfully, but custom config not applied.\n');
              resolve({ success: true, output, warning: 'SPIFFS upload failed' });
            });
        } else {
          // Upload standard SPIFFS (includes all files from data/ directory)
          event.sender.send('flash-progress', '\n=== Uploading SPIFFS filesystem ===\n');
          uploadSPIFFS(event, projectPath, envName, port, pioCmd, env, useShell, workingDir)
            .then(() => {
              resolve({ success: true, output });
            })
            .catch((error) => {
              event.sender.send('flash-progress', `⚠ SPIFFS upload failed: ${error.message}\n`);
              event.sender.send('flash-progress', 'Firmware uploaded successfully, but SPIFFS files not uploaded.\n');
              resolve({ success: true, output, warning: 'SPIFFS upload failed' });
            });
        }
      } else {
        reject(new Error(`PlatformIO build/upload failed with code ${code}\n${output}`));
      }
    });
    
    pio.on('error', (err) => {
      reject(new Error(`Failed to start PlatformIO: ${err.message}`));
    });
  });
}

// Upload SPIFFS using PlatformIO uploadfs (standard upload, no custom config)
// This ensures all files from data/ directory are included (index.html, style.css, app.js, etc.)
async function uploadSPIFFS(event, projectPath, envName, port, pioCmd, env, useShell, workingDir) {
  try {
    // Set upload port for PlatformIO
    const uploadEnv = { ...env };
    uploadEnv.PLATFORMIO_UPLOAD_PORT = port;
    
    // Run PlatformIO uploadfs to upload SPIFFS (includes all files from data/ directory)
    event.sender.send('flash-progress', 'Uploading SPIFFS filesystem (includes all web files)...\n');
    return new Promise((resolve, reject) => {
      const uploadfsArgs = ['run', '-e', envName, '-t', 'uploadfs', '--verbose'];
      const pio = spawn(pioCmd, uploadfsArgs, {
        cwd: workingDir,
        env: uploadEnv,
        shell: useShell,
        stdio: ['ignore', 'pipe', 'pipe']
      });
      
      let output = '';
      
      pio.stdout.on('data', (data) => {
        const text = data.toString();
        output += text;
        event.sender.send('flash-progress', text);
      });
      
      pio.stderr.on('data', (data) => {
        const text = data.toString();
        output += text;
        event.sender.send('flash-progress', text);
      });
      
      pio.on('close', (code) => {
        if (code === 0) {
          event.sender.send('flash-progress', '✓ SPIFFS uploaded successfully (all web files included)\n');
          resolve();
        } else {
          reject(new Error(`SPIFFS upload failed with code ${code}\n${output}`));
        }
      });
      
      pio.on('error', (err) => {
        reject(new Error(`Failed to start SPIFFS upload: ${err.message}`));
      });
    });
  } catch (error) {
    throw new Error(`Failed to prepare SPIFFS upload: ${error.message}`);
  }
}

// Upload SPIFFS from pre-built spiffs.bin using PlatformIO's esptool wrapper
// PlatformIO will read the SPIFFS address from the partition table automatically
async function uploadSPIFFSFromPrebuilt(event, firmwarePath, boardType, port, spiffsBinPath) {
  const pioCmd = findPlatformIO();
  if (!pioCmd) {
    throw new Error('PlatformIO not found. Cannot upload SPIFFS using PlatformIO method.');
  }
  
  // Map board type to PlatformIO environment name
  const envMap = {
    'esp32-c3-supermini': 'esp32-c3-supermini',
    'nuclearcounter': 'nuclearcounter',
    'esp32-c3': 'esp32-c3',
    'esp32-c6': 'esp32-c6',
    'esp32dev': 'esp32dev',
    'esp32-s3': 'esp32-s3',
    'esp32-s3-touch': 'esp32-s3-touch',
    'jc2432w328c': 'jc2432w328c'
  };
  
  const envName = envMap[boardType];
  if (!envName) {
    throw new Error(`Unknown board type: ${boardType}`);
  }
  
  // Create temporary directory for PlatformIO project
  const tempDir = require('path').join(require('os').tmpdir(), `sfos-pio-${Date.now()}`);
  const tempDataDir = require('path').join(tempDir, 'data');
  
  try {
    // Create temp directory structure
    await fs.mkdir(tempDataDir, { recursive: true });
    
    // Copy partitions.bin to temp directory (PlatformIO needs it to read SPIFFS address)
    const partitionsPath = require('path').join(firmwarePath, 'partitions.bin');
    if (require('fs').existsSync(partitionsPath)) {
      await fs.copyFile(partitionsPath, require('path').join(tempDir, 'partitions.bin'));
    }
    
    // Extract spiffs.bin to data/ directory using Python script
    const inspectScript = require('path').join(__dirname, 'scripts', 'inspect_spiffs.py');
    if (!require('fs').existsSync(inspectScript)) {
      throw new Error('SPIFFS extraction script not found. Cannot extract spiffs.bin to data/ directory.');
    }
    
    event.sender.send('flash-progress', 'Extracting SPIFFS files to data/ directory...\n');
    const { spawn } = require('child_process');
    await new Promise((resolve, reject) => {
      const python = spawn('python3', [inspectScript, spiffsBinPath, '-e', tempDataDir]);
      let output = '';
      python.stdout.on('data', (data) => {
        const text = data.toString();
        output += text;
        event.sender.send('flash-progress', text);
      });
      python.stderr.on('data', (data) => {
        const text = data.toString();
        output += text;
        event.sender.send('flash-progress', text);
      });
      python.on('close', (code) => {
        if (code === 0) {
          event.sender.send('flash-progress', '✓ SPIFFS files extracted\n');
          resolve();
        } else {
          reject(new Error(`Failed to extract SPIFFS: ${output}`));
        }
      });
      python.on('error', (err) => {
        reject(new Error(`Failed to run extraction script: ${err.message}`));
      });
    });
    
    // Create minimal platformio.ini
    const platformioIni = `[env:${envName}]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip
framework = arduino
board_build.filesystem = spiffs
`;
    await fs.writeFile(require('path').join(tempDir, 'platformio.ini'), platformioIni);
    
    // Set upload port
    const uploadEnv = { ...process.env };
    uploadEnv.PLATFORMIO_UPLOAD_PORT = port;
    
    // Run PlatformIO uploadfs - this will read the SPIFFS address from partitions.bin
    event.sender.send('flash-progress', 'Uploading SPIFFS using PlatformIO (reading address from partition table)...\n');
    return new Promise((resolve, reject) => {
      const uploadfsArgs = ['run', '-e', envName, '-t', 'uploadfs', '--verbose'];
      const pio = spawn(pioCmd, uploadfsArgs, {
        cwd: tempDir,
        env: uploadEnv,
        shell: process.platform === 'win32',
        stdio: ['ignore', 'pipe', 'pipe']
      });
      
      let output = '';
      pio.stdout.on('data', (data) => {
        const text = data.toString();
        output += text;
        event.sender.send('flash-progress', text);
      });
      pio.stderr.on('data', (data) => {
        const text = data.toString();
        output += text;
        event.sender.send('flash-progress', text);
      });
      pio.on('close', (code) => {
        if (code === 0) {
          resolve();
        } else {
          reject(new Error(`PlatformIO uploadfs failed with code ${code}\n${output}`));
        }
      });
      pio.on('error', (err) => {
        reject(new Error(`Failed to start PlatformIO uploadfs: ${err.message}`));
      });
    });
  } finally {
    // Clean up temp directory
    try {
      await fs.rm(tempDir, { recursive: true, force: true });
    } catch (err) {
      // Ignore cleanup errors
    }
  }
}

// Upload SPIFFS with custom config using PlatformIO uploadfs
// This ensures config.json is included along with web files (index.html, style.css, app.js)
async function uploadSPIFFSWithConfig(event, projectPath, envName, port, customConfig, pioCmd, env, useShell, workingDir) {
  const dataDir = path.join(projectPath, 'data');
  const configJsonPath = path.join(dataDir, 'config.json');
  let configFileCreated = false;
  
  try {
    // Ensure data directory exists
    await fs.mkdir(dataDir, { recursive: true });
    
    // Write config.json to data directory
    event.sender.send('flash-progress', 'Writing config.json to data/ directory...\n');
    await fs.writeFile(configJsonPath, JSON.stringify(customConfig, null, 2));
    configFileCreated = true;
    event.sender.send('flash-progress', '✓ config.json created\n');
    
    // Set upload port for PlatformIO
    const uploadEnv = { ...env };
    uploadEnv.PLATFORMIO_UPLOAD_PORT = port;
    
    // Run PlatformIO uploadfs to upload SPIFFS (includes data/ directory contents)
    event.sender.send('flash-progress', 'Uploading SPIFFS filesystem (includes web files + config.json)...\n');
    return new Promise((resolve, reject) => {
      const uploadfsArgs = ['run', '-e', envName, '-t', 'uploadfs', '--verbose'];
      const pio = spawn(pioCmd, uploadfsArgs, {
        cwd: workingDir,
        env: uploadEnv,
        shell: useShell,
        stdio: ['ignore', 'pipe', 'pipe']
      });
      
      let output = '';
      
      pio.stdout.on('data', (data) => {
        const text = data.toString();
        output += text;
        event.sender.send('flash-progress', text);
      });
      
      pio.stderr.on('data', (data) => {
        const text = data.toString();
        output += text;
        event.sender.send('flash-progress', text);
      });
      
      pio.on('close', (code) => {
        if (code === 0) {
          event.sender.send('flash-progress', '✓ SPIFFS uploaded successfully (web files + config.json)\n');
          resolve();
        } else {
          reject(new Error(`SPIFFS upload failed with code ${code}\n${output}`));
        }
      });
      
      pio.on('error', (err) => {
        reject(new Error(`Failed to start SPIFFS upload: ${err.message}`));
      });
    });
  } catch (error) {
    throw new Error(`Failed to prepare SPIFFS upload: ${error.message}`);
  } finally {
    // Clean up: remove config.json from data/ directory if we created it
    // This prevents it from being included in future builds unless explicitly set
    if (configFileCreated) {
      try {
        await fs.unlink(configJsonPath);
        event.sender.send('flash-progress', '✓ Cleaned up config.json from data/ directory\n');
      } catch (err) {
        // Ignore cleanup errors - file might not exist or be locked
        event.sender.send('flash-progress', `⚠ Could not remove config.json (non-critical): ${err.message}\n`);
      }
    }
  }
}

// Upload custom SPIFFS using esptool (fallback for GitHub releases)
async function uploadCustomSPIFFS(event, projectPath, envName, port, customConfig, boardType) {
  // Generate SPIFFS image
  event.sender.send('flash-progress', 'Generating custom SPIFFS image...\n');
  const spiffsPath = await generateCustomSPIFFS(customConfig);
  
  // Use esptool to upload SPIFFS to the correct address
  const config = BOARD_CONFIGS[boardType];
  
  if (!config) {
    throw new Error('Could not determine board config for SPIFFS upload');
  }
  
  let esptoolCmd;
  try {
    esptoolCmd = findEsptool();
  } catch (error) {
    throw new Error('esptool not found for SPIFFS upload');
  }
  
  return new Promise((resolve, reject) => {
    const args = [
      '--chip', config.chip,
      '--port', port,
      '--baud', '921600',
      '--before', 'default_reset',
      '--after', 'hard_reset',
      'write_flash',
      config.flashAddresses.spiffs,
      spiffsPath
    ];
    
    const esptool = spawn(esptoolCmd, args);
    let output = '';
    
    esptool.stdout.on('data', (data) => {
      const text = data.toString();
      output += text;
      event.sender.send('flash-progress', text);
    });
    
    esptool.stderr.on('data', (data) => {
      output += data.toString();
      event.sender.send('flash-progress', data.toString());
    });
    
    esptool.on('close', (code) => {
      if (code === 0) {
        event.sender.send('flash-progress', '✓ Custom SPIFFS uploaded successfully\n');
        resolve();
      } else {
        reject(new Error(`SPIFFS upload failed: ${output}`));
      }
    });
    
    esptool.on('error', (err) => {
      reject(new Error(`Failed to upload SPIFFS: ${err.message}`));
    });
  });
}

// Erase flash
ipcMain.handle('erase-flash', async (event, options) => {
  const { port, boardType } = options;
  const config = BOARD_CONFIGS[boardType];
  
  return new Promise((resolve, reject) => {
    let esptoolCmd;
    try {
      esptoolCmd = findEsptool();
    } catch (error) {
      reject(error);
      return;
    }
    
    const args = [
      '--chip', config.chip,
      '--port', port,
      'erase_flash'
    ];
    
    const esptool = spawn(esptoolCmd, args);
    let output = '';
    
    esptool.stdout.on('data', (data) => {
      output += data.toString();
      event.sender.send('erase-progress', data.toString());
    });
    
    esptool.stderr.on('data', (data) => {
      output += data.toString();
    });
    
    esptool.on('close', (code) => {
      if (code === 0) {
        resolve({ success: true, output });
      } else {
        reject(new Error(`Erase failed with code ${code}\n${output}`));
      }
    });
  });
});

// Select local firmware folder
ipcMain.handle('select-firmware-folder', async () => {
  const result = await dialog.showOpenDialog(mainWindow, {
    properties: ['openDirectory'],
    title: 'Select Firmware Folder'
  });
  
  if (result.canceled) {
    return null;
  }
  
  return result.filePaths[0];
});

