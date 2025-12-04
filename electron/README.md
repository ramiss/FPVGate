# FPVGate Desktop App (Electron)

Desktop application for FPVGate Lap Timer with WiFi and USB connectivity.

## Features

- ✅ **WiFi & USB Toggle** - Switch between connection modes in-app
- ✅ **Auto-detect** - Automatically finds FPVGate USB device
- ✅ **Smart Default** - Falls back to USB if WiFi unavailable
- ✅ **Native App** - Runs as Windows desktop application
- ✅ **Same UI** - Uses your existing web interface files from `data/` folder
- ✅ **Live Updates** - Edit files in `data/` and restart app to see changes

## Setup

### Install Dependencies

```bash
cd electron
npm install
```

This will install:
- `electron` - Desktop app framework
- `serialport` - USB serial communication
- `electron-builder` - Build/packaging tool

## Development

### Run the App

```bash
npm start
```

This launches the Electron app in development mode. It will:
1. Load your web interface from `../data/index.html`
2. Try to connect via WiFi (default mode)
3. Allow switching to USB via the connection selector

### Making Changes

1. Edit files in `../data/` (index.html, script.js, etc.)
2. Restart the Electron app (Ctrl+C and `npm start`)
3. Changes are reflected immediately!

## Building for Distribution

### Build Windows Installer

```bash
npm run build:win
```

This creates:
- `dist/FPVGate Setup 1.0.0.exe` - Windows installer
- Installs to `C:\Program Files\FPVGate\`
- Creates desktop shortcut
- Creates start menu entry

### Build Output

```
electron/dist/
  FPVGate Setup 1.0.0.exe    (~80MB installer)
  win-unpacked/              (portable version)
```

## How It Works

### File Structure

```
FPVGate/
├── electron/
│   ├── main.js           → Electron main process (Node.js)
│   ├── preload.js        → Secure IPC bridge
│   ├── package.json      → Dependencies & build config
│   └── README.md         → This file
└── data/
    ├── index.html        → Web interface (loaded by Electron)
    ├── script.js         → UI logic
    ├── usb-transport.js  → USB/WiFi transport layer
    └── ...               → Other web files
```

### Connection Logic

1. **App Starts** → Loads `data/index.html` in Electron window
2. **Default Mode** → WiFi (tries to connect to `http://192.168.4.1`)
3. **WiFi Fails?** → Shows connection error, user can switch to USB
4. **USB Mode** → Auto-detects COM port, connects via Node.js serialport
5. **Toggle Anytime** → User can switch between WiFi/USB in the UI

### WiFi vs USB

| Feature | WiFi Mode | USB Mode |
|---------|-----------|----------|
| **Connection** | HTTP + SSE | Serial port (COM) |
| **Latency** | ~50-100ms | ~10ms |
| **Setup** | Need to connect to FPVGate WiFi | Plug in USB cable |
| **Range** | ~30m | Cable length |
| **Auto-detect** | No (fixed IP) | Yes (auto-finds ESP32) |

## Troubleshooting

### "Cannot find FPVGate device"

**Solution:** 
- Make sure FPVGate is plugged in via USB
- Check Device Manager for COM port
- ESP32-S3 should show as "USB-SERIAL CH340" or "Espressif" device

### "Permission denied" on serial port

**Solution:**
- Close any other program using the serial port (Arduino IDE, PuTTY, etc.)
- Reconnect the USB cable
- Try switching to WiFi and back to USB

### WiFi not connecting

**Solution:**
- Make sure you're connected to FPVGate WiFi network
- Check FPVGate IP is `192.168.4.1` (or update in code)
- Switch to USB mode instead

### App not starting

**Solution:**
```bash
# Reinstall dependencies
cd electron
rm -rf node_modules
npm install
npm start
```

### Changes not showing up

**Solution:**
- Make sure you're editing files in the `data/` folder (not `electron/` folder)
- Fully restart the app (Ctrl+C then `npm start` again)
- Clear cache: Delete `%APPDATA%/fpvgate-desktop/` folder

## Distribution

### For End Users

1. Build the installer: `npm run build:win`
2. Distribute `FPVGate Setup 1.0.0.exe`
3. Users run installer
4. App appears on desktop and Start menu

### Portable Version

The `dist/win-unpacked/` folder contains a portable version:
- No installation needed
- Just zip it and distribute
- Extract and run `FPVGate.exe`

## Advanced Configuration

### Change App Name/Icon

Edit `package.json`:
```json
{
  "build": {
    "productName": "FPVGate",
    "appId": "com.fpvgate.laptimer",
    "win": {
      "icon": "../data/logo-white.svg"
    }
  }
}
```

### Change Default WiFi IP

Edit `../data/script.js` or create a config file.

### Enable DevTools

Uncomment in `main.js`:
```javascript
mainWindow.webContents.openDevTools();
```

## Development Tips

### Hot Reload (Advanced)

Install `electron-reload`:
```bash
npm install --save-dev electron-reload
```

Add to `main.js`:
```javascript
require('electron-reload')(__dirname, {
  electron: path.join(__dirname, 'node_modules', '.bin', 'electron')
});
```

### Debug Serial Communication

Check console in DevTools for:
- `[USB] →` - Commands sent
- `[USB] ←` - Responses received
- `[USB Debug]` - Non-JSON serial data

## Building for macOS/Linux

### macOS
```bash
npm run build -- --mac
```

Requires:
- macOS machine
- Apple Developer certificate (for signing)

### Linux
```bash
npm run build -- --linux
```

Creates `.AppImage` or `.deb` package.

## License

Same as FPVGate main project (MIT).
