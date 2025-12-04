const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');

let mainWindow;
let serialPort = null;
let parser = null;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1400,
    height: 900,
    icon: path.join(__dirname, '../data/logo-white.svg'),
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, 'preload.js'),
      // Enable media APIs for audio announcements
      enableRemoteModule: false,
      sandbox: false
    },
    title: 'FPVGate Lap Timer',
    backgroundColor: '#1a1a2e'
  });

  // Load the app from data folder
  // In development: ../data/index.html
  // In packaged app: resources/app/../data/index.html
  const isDev = !app.isPackaged;
  const dataPath = isDev 
    ? path.join(__dirname, '../data/index.html')
    : path.join(process.resourcesPath, 'data/index.html');
  
  console.log('Loading from:', dataPath);
  mainWindow.loadFile(dataPath).catch(err => {
    console.error('Failed to load:', err);
    // Fallback: try relative path
    mainWindow.loadFile(path.join(__dirname, '../data/index.html'));
  });

  // Open DevTools in development
  // mainWindow.webContents.openDevTools();

  // Grant permissions for media devices (for audio announcements)
  mainWindow.webContents.session.setPermissionRequestHandler((webContents, permission, callback) => {
    const allowedPermissions = ['media', 'audioCapture', 'audioPlayback'];
    if (allowedPermissions.includes(permission)) {
      callback(true); // Approve
    } else {
      callback(false); // Deny
    }
  });

  mainWindow.on('closed', () => {
    if (serialPort && serialPort.isOpen) {
      serialPort.close();
    }
    mainWindow = null;
  });
}

app.whenReady().then(createWindow);

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow();
  }
});

// IPC Handlers for USB communication

// List available serial ports
ipcMain.handle('list-ports', async () => {
  try {
    const ports = await SerialPort.list();
    return ports.map(port => ({
      path: port.path,
      manufacturer: port.manufacturer,
      serialNumber: port.serialNumber,
      pnpId: port.pnpId,
      vendorId: port.vendorId,
      productId: port.productId
    }));
  } catch (error) {
    console.error('Error listing ports:', error);
    return [];
  }
});

// Connect to serial port
ipcMain.handle('connect-serial', async (event, portPath) => {
  try {
    if (serialPort && serialPort.isOpen) {
      await serialPort.close();
    }

    serialPort = new SerialPort({
      path: portPath,
      baudRate: 115200
    });

    parser = serialPort.pipe(new ReadlineParser({ delimiter: '\n' }));

    // Forward serial data to renderer
    parser.on('data', (line) => {
      if (mainWindow && !mainWindow.isDestroyed()) {
        mainWindow.webContents.send('serial-data', line.trim());
      }
    });

    serialPort.on('error', (err) => {
      console.error('Serial port error:', err);
      if (mainWindow && !mainWindow.isDestroyed()) {
        mainWindow.webContents.send('serial-error', err.message);
      }
    });

    serialPort.on('close', () => {
      if (mainWindow && !mainWindow.isDestroyed()) {
        mainWindow.webContents.send('serial-disconnected');
      }
    });

    return { success: true };
  } catch (error) {
    console.error('Error connecting to serial:', error);
    return { success: false, error: error.message };
  }
});

// Disconnect serial port
ipcMain.handle('disconnect-serial', async () => {
  try {
    if (serialPort && serialPort.isOpen) {
      await serialPort.close();
    }
    serialPort = null;
    parser = null;
    return { success: true };
  } catch (error) {
    console.error('Error disconnecting serial:', error);
    return { success: false, error: error.message };
  }
});

// Write to serial port
ipcMain.handle('write-serial', async (event, data) => {
  try {
    if (!serialPort || !serialPort.isOpen) {
      throw new Error('Serial port not connected');
    }
    
    serialPort.write(data + '\n');
    return { success: true };
  } catch (error) {
    console.error('Error writing to serial:', error);
    return { success: false, error: error.message };
  }
});

// Check serial connection status
ipcMain.handle('serial-status', async () => {
  return {
    connected: serialPort !== null && serialPort.isOpen,
    path: serialPort ? serialPort.path : null
  };
});
