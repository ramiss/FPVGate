# FPVGate v1.4.0 Release Notes

**Release Date:** December 12, 2024  
**Platform:** ESP32-S3  
**Firmware Size:** RAM 23.3% (76,328 bytes), Flash 58.6% (1,228,809 bytes)

---

## 🎉 Major New Features

### Webhook System for External LED Controllers

FPVGate can now send HTTP webhooks to external devices on your network, perfect for triggering gate LED flashes and synchronized lighting effects.

**Features:**
- Configure up to 10 webhook target IP addresses
- HTTP POST requests to network devices
- Event endpoints: `/RaceStart`, `/RaceStop`, `/Lap`, `/flash`, `/off`
- 500ms timeout per webhook
- Fire-and-forget delivery (non-blocking)

**Configuration:**
- **Configuration → Webhooks** - Add/remove IP addresses, enable/disable system
- **Configuration → LED Setup → Gate LEDs** - Granular control over which events trigger webhooks

**Use Cases:**
- External gate LED controllers
- Synchronized lighting systems
- Home automation integration
- Multi-gate setups

### Gate LED Controls

Granular control over when webhooks fire during race events.

**Master Control:**
- Enable/disable Gate LEDs system in LED Setup section

**Individual Event Toggles:**
- **Race Start Flash** - Send `/RaceStart` webhook (2× Green flashes)
- **Race Stop Flash** - Send `/RaceStop` webhook (2× Red flashes)
- **Lap Flash** - Send `/Lap` webhook (White flash on each lap)

**Behavior:**
- Both master switch AND individual toggle must be enabled for webhook to fire
- Settings persist to EEPROM (survive reboots)
- Manual lap additions also trigger `/Lap` webhook when enabled

### Enhanced Self-Test System

Comprehensive diagnostic system expanded from 6 to 19 tests.

**New Tests:**
- **Track Manager** - Verifies track file existence and storage
- **Webhooks** - Checks HTTP client availability
- **Transport Layer** - Tests WiFi/USB connectivity and transport JS
- **Enhanced SD Card** - R/W verification, voice files, free space reporting

**Test Categories:**
- **Hardware** (4 tests): RX5808, RGB LED, Battery, Buzzer
- **Storage** (4 tests): EEPROM, LittleFS, SD Card, Storage Manager
- **Connectivity** (3 tests): WiFi, USB, Transport
- **Software** (8 tests): Config, LapTimer, RaceHistory, TrackManager, Webhooks, WebServer, OTA

**Improvements:**
- Better error messages with actual values
- Organized category display
- Detailed diagnostic feedback

---

## 🔧 Backend Changes

### WebhookManager Class
- New library: `lib/WEBHOOK/`
- HTTP client for POST requests
- IP address management (add/remove/clear)
- Trigger functions for all race events

### Configuration System
- **EEPROM size increased:** 256 → 512 bytes
- **Config version bumped:** 4 → 5
- **New fields:**
  - `webhooksEnabled` - Master webhook toggle
  - `webhookIPs[10]` - Array of webhook IP addresses
  - `webhookCount` - Number of configured webhooks
  - `gateLEDsEnabled` - Gate LED master toggle
  - `webhookRaceStart` - Race start event toggle
  - `webhookRaceStop` - Race stop event toggle
  - `webhookLap` - Lap event toggle

### Lap Timer
- Conditional webhook triggers
- Checks both master switch and individual toggles before firing webhooks
- Manual laps also trigger webhooks when enabled

### WiFi System
- **Fixed:** WiFi apply button now properly reboots device
- Changed endpoint from `/restart` to `/reboot`

---

## 🎨 Frontend Changes

### Webhooks Section
- New section in Configuration modal
- Enable/disable master toggle
- Add webhook IP input with validation
- Webhook list with remove buttons
- Test button (sends `/flash` to all configured webhooks)

### Gate LEDs Section
- New subsection in LED Setup
- Master "Enable Gate LEDs" toggle
- Individual toggles for Race Start/Stop/Lap events
- Settings auto-save to EEPROM
- Show/hide options based on master toggle state

### Configuration Modal
- Improved organization with 6 sections:
  1. General - Pilot, frequency, RSSI, race settings
  2. Tracks - Track management and selection
  3. LED Setup - Visual effects and Gate LEDs
  4. WiFi & Connection - Network settings
  5. **Webhooks** - External integration (NEW)
  6. System Settings - Theme, battery, diagnostics

---

## 📡 API Endpoints

### Webhooks API

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/webhooks` | GET | List configured webhooks and status |
| `/webhooks/add` | POST | Add webhook IP address |
| `/webhooks/remove` | POST | Remove webhook IP address |
| `/webhooks/clear` | POST | Clear all webhooks |
| `/webhooks/enable` | POST | Enable/disable webhook system |
| `/webhooks/trigger/flash` | POST | Manual test trigger |

### Webhook Triggers (Sent to External Devices)

| Endpoint | Trigger | Description |
|----------|---------|-------------|
| `/RaceStart` | Race begins | Sent when Start Race clicked |
| `/RaceStop` | Race ends | Sent when Stop Race clicked |
| `/Lap` | Lap detected | Sent on each lap (including manual) |
| `/flash` | Manual test | Triggered by Test button |
| `/off` | Manual | Turn off LEDs |

---

## 📚 Documentation Updates

### README.md
- Added "Webhooks & Integration" section
- Updated v1.4.0 release notes
- Mentioned 19 comprehensive self-tests

### FEATURES.md
- New "Track Management" section (~50 lines)
- New "Webhooks & Integration" section (200+ lines)
  - Complete API documentation
  - Example LED controller code
  - Network requirements
  - Troubleshooting guide
- Updated "Self-Test System" section
  - All 19 tests documented
  - Organized into categories

---

## 🐛 Bug Fixes

- **WiFi Apply Button:** Now properly reboots device to apply settings (was calling `/restart`, now calls `/reboot`)
- **Configuration Test:** Error messages now show actual RSSI values for better debugging

---

## 🔄 Migration Notes

### From v1.3.x

**Config Reset:**
- Config version bump will trigger automatic reset to defaults
- **Action Required:** Re-enter your settings after first boot
- Settings affected: Frequency, pilot info, RSSI thresholds, etc.

**EEPROM Size:**
- EEPROM size increased from 256 to 512 bytes
- Accommodates webhook IPs and new config fields
- No user action required

**New Features (Optional):**
- Webhooks are **disabled by default**
- Gate LEDs are **disabled by default**
- Individual webhook toggles are **enabled by default** (when Gate LEDs enabled)

---

## 📦 Release Files

### ESP32-S3 Binaries
- `bootloader-esp32s3.bin` - ESP32-S3 bootloader (15,104 bytes)
- `partitions-esp32s3.bin` - Partition table (3,072 bytes)
- `firmware-esp32s3.bin` - Main firmware (1,228,809 bytes)
- `littlefs-esp32s3.bin` - Web interface filesystem (1,048,576 bytes)

### Desktop App
- `FPVGate_Desktop_v1.4.0_Windows_x64.zip` - Windows 64-bit Electron app

---

## 🚀 Flashing Instructions

### Full Install (New Device)

```bash
esptool.py --chip esp32s3 --port COM3 write_flash -z \
  0x0 bootloader-esp32s3.bin \
  0x8000 partitions-esp32s3.bin \
  0x10000 firmware-esp32s3.bin \
  0x410000 littlefs-esp32s3.bin
```

### Update Firmware Only

```bash
esptool.py --chip esp32s3 --port COM3 write_flash -z \
  0x10000 firmware-esp32s3.bin
```

### Update Web Interface Only

```bash
esptool.py --chip esp32s3 --port COM3 write_flash -z \
  0x410000 littlefs-esp32s3.bin
```

**Note:** Replace `COM3` with your actual port.

---

## ⚙️ Example LED Controller Setup

Quick ESP8266/ESP32 example for external gate LEDs:

```cpp
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FastLED.h>

#define LED_PIN D4
#define NUM_LEDS 30
CRGB leds[NUM_LEDS];

const char* ssid = "FPVGate_XXXX";
const char* password = "fpvgate1";

ESP8266WebServer server(80);

void setup() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(100);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());  // Add this IP to FPVGate
  
  server.on("/RaceStart", HTTP_POST, []() {
    // Flash green 2x
    for(int i=0; i<2; i++) {
      fill_solid(leds, NUM_LEDS, CRGB::Green);
      FastLED.show();
      delay(200);
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
      delay(100);
    }
    server.send(200, "text/plain", "OK");
  });
  
  server.on("/Lap", HTTP_POST, []() {
    // Flash white 0.5s
    fill_solid(leds, NUM_LEDS, CRGB::White);
    FastLED.show();
    delay(500);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    server.send(200, "text/plain", "OK");
  });
  
  server.begin();
}

void loop() {
  server.handleClient();
}
```

---

## 🧪 Testing Status

✅ Firmware builds successfully  
✅ Flashed to ESP32-S3  
✅ Webhook system functional  
✅ Gate LED toggles working  
✅ Self-tests complete (19/19 tests)  
✅ Settings persist to EEPROM  
✅ WiFi apply button reboots device  
✅ Manual laps trigger webhooks  

---

## 🙏 Acknowledgments

Thank you to the FPV community for feedback and feature requests!

---

## 📞 Support

- **Issues:** https://github.com/LouisHitchcock/FPVGate/issues
- **Discussions:** https://github.com/LouisHitchcock/FPVGate/discussions
- **Documentation:** https://github.com/LouisHitchcock/FPVGate/tree/main/docs

---

**Full Changelog:** https://github.com/LouisHitchcock/FPVGate/blob/main/CHANGELOG.md
