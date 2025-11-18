const { app, BrowserWindow, ipcMain, dialog } = require('electron');
const path = require('path');
const { spawn } = require('child_process');
const fs = require('fs').promises;
const { SerialPort } = require('serialport');
const axios = require('axios');
const extractZip = require('extract-zip');

let mainWindow;

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
    return 'esp32-c3-supermini'; // Most common C3 board
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
    // Fetch all releases (including pre-releases), then get the first one
    const response = await axios.get(
      'https://api.github.com/repos/RaceFPV/StarForgeOS/releases',
      {
        headers: {
          'Accept': 'application/vnd.github.v3+json',
          'User-Agent': 'StarForge-Flasher'
        }
      }
    );
    
    if (!response.data || response.data.length === 0) {
      throw new Error('No releases found');
    }
    
    // Get the first release (most recent, including pre-releases)
    const latestRelease = response.data[0];
    
    return {
      tag: latestRelease.tag_name,
      name: latestRelease.name,
      assets: latestRelease.assets.map(asset => ({
        name: asset.name,
        url: asset.browser_download_url,
        size: asset.size
      }))
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
    
    const zipPath = path.join(cacheDir, `${boardType}.zip`);
    const extractPath = path.join(cacheDir, boardType);
    
    // Download
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
    
    // Extract
    await extractZip(zipPath, { dir: extractPath });
    
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
    
    // Add flash addresses and files
    const files = {
      bootloader: path.join(firmwarePath, 'bootloader.bin'),
      partitions: path.join(firmwarePath, 'partitions.bin'),
      firmware: path.join(firmwarePath, 'firmware.bin'),
      spiffs: path.join(firmwarePath, 'spiffs.bin')
    };
    
    // Check which files exist
    const existingFiles = [];
    Object.keys(files).forEach(key => {
      if (require('fs').existsSync(files[key])) {
        existingFiles.push(key);
        args.push(config.flashAddresses[key], files[key]);
      }
    });
    
    // If no firmware.bin found, that's an error
    if (!existingFiles.includes('firmware')) {
      reject(new Error('No firmware files found, expected firmware.bin. If this is source code, make sure platformio.ini exists in the folder.'));
      return;
    }
    
    // If custom config is provided, generate and add custom SPIFFS
    if (customConfig) {
      try {
        event.sender.send('flash-progress', '\n=== Generating custom config SPIFFS ===\n');
        const customSPIFFSPath = await generateCustomSPIFFS(customConfig);
        event.sender.send('flash-progress', `✓ Custom SPIFFS generated: ${customSPIFFSPath}\n`);
        
        // Add custom SPIFFS to flash command (overwrites default spiffs)
        args.push(config.flashAddresses.spiffs, customSPIFFSPath);
      } catch (error) {
        event.sender.send('flash-progress', `⚠ Custom SPIFFS generation failed: ${error.message}\n`);
        event.sender.send('flash-progress', 'Continuing with standard firmware (no custom pins)\n');
      }
    }
    
    // Spawn esptool
    const esptool = spawn(esptoolCmd, args);
    
    let output = '';
    
    esptool.stdout.on('data', (data) => {
      const text = data.toString();
      output += text;
      event.sender.send('flash-progress', text);
      
      // Parse progress from esptool output
      const progressMatch = text.match(/Writing at 0x[0-9a-f]+\.\.\. \((\d+) %\)/);
      if (progressMatch) {
        event.sender.send('flash-percent', parseInt(progressMatch[1]));
      }
    });
    
    esptool.stderr.on('data', (data) => {
      const text = data.toString();
      output += text;
      event.sender.send('flash-progress', text);
    });
    
    esptool.on('close', (code) => {
      if (code === 0) {
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
    
    // Fix Unicode encoding issues on Windows
    if (process.platform === 'win32') {
      env.PYTHONIOENCODING = 'utf-8';
      env.PYTHONUTF8 = '1';
      // Also set console codepage to UTF-8
      env.CHCP = '65001';
    }
    
    const buildArgs = ['run', '-e', envName, '-t', 'upload'];
    
    // On Windows with batch files, we need shell: true
    // But we'll use the workingDir with forward slashes which Node.js handles
    const pio = spawn(pioCmd, buildArgs, {
      cwd: workingDir,
      env: env,
      shell: useShell
    });
    
    let output = '';
    
    pio.stdout.on('data', (data) => {
      const text = data.toString();
      output += text;
      event.sender.send('flash-progress', text);
      
      // Parse progress from PlatformIO output
      const progressMatch = text.match(/(\d+)%/);
      if (progressMatch) {
        event.sender.send('flash-percent', parseInt(progressMatch[1]));
      }
    });
    
    pio.stderr.on('data', (data) => {
      const text = data.toString();
      output += text;
      event.sender.send('flash-progress', text);
    });
    
    pio.on('close', (code) => {
      if (code === 0) {
        // If custom config is provided, upload SPIFFS separately
        if (customConfig) {
          event.sender.send('flash-progress', '\n=== Generating and uploading custom SPIFFS ===\n');
          uploadCustomSPIFFS(event, projectPath, envName, port, customConfig, boardType)
            .then(() => {
              resolve({ success: true, output });
            })
            .catch((error) => {
              event.sender.send('flash-progress', `⚠ Custom SPIFFS upload failed: ${error.message}\n`);
              event.sender.send('flash-progress', 'Firmware uploaded successfully, but custom config not applied.\n');
              resolve({ success: true, output, warning: 'Custom SPIFFS upload failed' });
            });
        } else {
          resolve({ success: true, output });
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

// Upload custom SPIFFS using PlatformIO
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

