#include "standalone_mode.h"
#include "config_globals.h"
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include <string.h>
#include "config.h"

#if defined(BOARD_NUCLEARCOUNTER)
#include <Wire.h>
#include <U8g2lib.h>
#endif

#if ENABLE_AUDIO && defined(BOARD_JC2432W328C)
// SimpleTTS support - includes for future implementation
// #include "SimpleTTS.h"
// #include "AudioTools.h"
// TODO: Implement custom AudioOutput class for DAC26 once audio files are ready
#endif

#if ENABLE_LCD_UI
StandaloneMode* StandaloneMode::_lcdInstance = nullptr;
#endif

StandaloneMode::StandaloneMode() : _server(80), _timingCore(nullptr) {
#if ENABLE_LCD_UI
    _lcdUI = nullptr;
    _lcdInstance = this;
#endif
}

void StandaloneMode::begin(TimingCore* timingCore) {
    _timingCore = timingCore;
    
#if ENABLE_BATTERY_MONITOR && defined(BATTERY_ADC_PIN)
    // Initialize ADC for battery monitoring
    pinMode(g_battery_adc_pin, INPUT);
    
    // Set attenuation specifically for this pin (ADC1)
    analogSetPinAttenuation(g_battery_adc_pin, ADC_11db);  // 0-3.3V range
    
    // Set ADC resolution (ESP32-S3 uses analogReadResolution instead of analogSetWidth)
    #if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_ESP32S3_DEV)
        analogReadResolution(12);  // 12-bit resolution (0-4095) for ESP32-S3
    #else
        analogSetWidth(12);  // 12-bit resolution (0-4095) for ESP32
    #endif
#endif

#if defined(USB_DETECT_PIN)
    // Initialize USB detection pin (D+ line monitoring)
    pinMode(g_usb_detect_pin, INPUT);
    Serial.printf("USB detection enabled on GPIO%d (connect D+ -> 100kΩ -> GPIO%d)\n", 
                  g_usb_detect_pin, g_usb_detect_pin);
#endif

#if ENABLE_AUDIO
    // Initialize audio DAC
    initAudio();
#endif
    
    // Initialize WiFi AP with stability improvements
    setupWiFiAP();
    
    // Give WiFi time to stabilize
    delay(600);
    
    // Initialize mDNS for .local hostname
    if (MDNS.begin(MDNS_HOSTNAME)) {
        Serial.printf("mDNS responder started: %s.local\n", MDNS_HOSTNAME);
        MDNS.addService("http", "tcp", WEB_SERVER_PORT);
    } else {
        Serial.println("Warning: Error setting up mDNS responder (not critical)");
    }
    
    // Initialize SPIFFS for serving static files
    // Check if already mounted (config_loader may have mounted it)
    // Try to mount, but don't format if already mounted
    bool spiffsMounted = false;
    if (SPIFFS.begin(false)) {
        // Already mounted, or mounted successfully without formatting
        spiffsMounted = true;
    } else {
        // Not mounted, try mounting with format (first time setup)
        spiffsMounted = SPIFFS.begin(true);
    }
    
    if (!spiffsMounted) {
        Serial.println("Warning: SPIFFS Mount Failed (index.html won't be available, but API will work)");
    } else {
        Serial.println("SPIFFS mounted successfully");
        
        // Get SPIFFS partition info
        size_t totalBytes = SPIFFS.totalBytes();
        size_t usedBytes = SPIFFS.usedBytes();
        Serial.printf("SPIFFS Partition: %d bytes total, %d bytes used, %d bytes free\n", 
                     totalBytes, usedBytes, totalBytes - usedBytes);
        
        // Debug: List all files in SPIFFS
        Serial.println("=== SPIFFS Contents ===");
        File root = SPIFFS.open("/");
        if (!root) {
            Serial.println("ERROR: Failed to open SPIFFS root directory");
        } else if (!root.isDirectory()) {
            Serial.println("ERROR: SPIFFS root is not a directory");
        } else {
            File file = root.openNextFile();
            int fileCount = 0;
            while (file) {
                Serial.printf("  File: %s, Size: %d bytes\n", file.name(), file.size());
                fileCount++;
                file = root.openNextFile();
            }
            if (fileCount == 0) {
                Serial.println("  WARNING: SPIFFS is empty! No files found.");
                Serial.println("  This means SPIFFS was not uploaded correctly or partition is empty.");
            } else {
                Serial.printf("Total files: %d\n", fileCount);
            }
        }
        Serial.println("======================");
    }
    
    // Setup web server routes
    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/api/status", HTTP_GET, [this]() { handleGetStatus(); });
    _server.on("/api/laps", HTTP_GET, [this]() { handleGetLaps(); });
    _server.on("/api/start_race", HTTP_POST, [this]() { handleStartRace(); });
    _server.on("/api/stop_race", HTTP_POST, [this]() { handleStopRace(); });
    _server.on("/api/clear_laps", HTTP_POST, [this]() { handleClearLaps(); });
    _server.on("/api/set_frequency", HTTP_POST, [this]() { handleSetFrequency(); });
    _server.on("/api/set_threshold", HTTP_POST, [this]() { handleSetThreshold(); });
    _server.on("/api/get_channels", HTTP_GET, [this]() { handleGetChannels(); });
    _server.on("/api/spiffs_info", HTTP_GET, [this]() { handleGetSPIFFSInfo(); });  // Debug endpoint
    _server.on("/style.css", HTTP_GET, [this]() { handleStyleCSS(); });
    _server.on("/app.js", HTTP_GET, [this]() { handleAppJS(); });
    _server.onNotFound([this]() { handleNotFound(); });
    
    _server.begin();
    Serial.println("Web server started");
    Serial.printf("Access point: %s\n", WIFI_AP_SSID_PREFIX);
    Serial.printf("IP address: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("mDNS hostname: %s.local\n", MDNS_HOSTNAME);
    Serial.println("Open browser to http://192.168.4.1 or http://sfos.local");

#if defined(BOARD_NUCLEARCOUNTER)
    initNuclearDisplay();
#endif
    
    // Load saved settings from flash BEFORE creating LCD UI and web server task
    // This ensures the correct values are displayed on boot
    loadSettings();
    
    // Create dedicated web server task
    // ESP32 dual-core: Pin to Core 0 (same as WiFi stack)
    // ESP32-C3/C6 (single core): No core pinning
#if defined(CONFIG_IDF_TARGET_ESP32C6) || defined(ARDUINO_ESP32C3_DEV) || defined(CONFIG_IDF_TARGET_ESP32C3)
    xTaskCreate(webServerTask, "WebServer", 8192, this, WEB_PRIORITY, &_webTaskHandle);
    Serial.println("Web server task created");
#else
    xTaskCreatePinnedToCore(webServerTask, "WebServer", 8192, this, WEB_PRIORITY, &_webTaskHandle, 0);
    Serial.println("Web server task created on Core 0");
#endif
    
#if ENABLE_LCD_UI
    // IMPORTANT: Give WiFi more time to fully stabilize before initializing LCD
    // LCD initialization uses I2C which can cause issues if WiFi isn't fully ready
    delay(500);
    
    // Initialize LCD UI (optional feature)
    Serial.println("\n====================================");
    Serial.println("Initializing LCD UI (optional)");
    Serial.println("====================================");
    
    _lcdUI = new LcdUI();
    if (_lcdUI && _lcdUI->begin()) {
        // Set button callbacks
        _lcdUI->setStartCallback(lcdStartCallback);
        _lcdUI->setStopCallback(lcdStopCallback);
        _lcdUI->setClearCallback(lcdClearCallback);
        _lcdUI->setSettingsChangedCallback([]() {
            // Called when user changes settings via LCD
            if (_lcdInstance) {
                _lcdInstance->saveSettings();
            }
        });
        _lcdUI->setTimingCore(_timingCore);  // Link timing core for settings access
        
        // Initialize settings display
        uint8_t band, channel;
        _timingCore->getRX5808Settings(band, channel);
        _lcdUI->updateBandChannel(band, channel);
        _lcdUI->updateFrequency(_timingCore->getCurrentFrequency());
        
        // Get and display current threshold from timing core
        uint8_t current_threshold = _timingCore->getThreshold();
        Serial.printf("LCD: Displaying threshold: %d (from config: %d)\n", 
                      current_threshold, CROSSING_THRESHOLD);
        _lcdUI->updateThreshold(current_threshold);
        
        // Create LCD UI task
        // ESP32 dual-core: Pin to Core 0 (with WiFi/web, low priority)
        // ESP32-C3/C6 (single core): No core pinning
        // Stack increased to 8KB for LVGL with full-screen rendering
#if defined(CONFIG_IDF_TARGET_ESP32C6) || defined(ARDUINO_ESP32C3_DEV) || defined(CONFIG_IDF_TARGET_ESP32C3)
        xTaskCreate(LcdUI::uiTask, "LcdUI", 8192, _lcdUI, LCD_PRIORITY, &_lcdTaskHandle);
        Serial.println("LCD UI task created");
#else
        xTaskCreatePinnedToCore(LcdUI::uiTask, "LcdUI", 8192, _lcdUI, LCD_PRIORITY, &_lcdTaskHandle, 0);
        Serial.println("LCD UI task created on Core 0");
#endif
    } else {
        Serial.println("Warning: LCD UI initialization failed (optional feature)");
        if (_lcdUI) {
            delete _lcdUI;
            _lcdUI = nullptr;
        }
    }
#endif
    
    Serial.println("Setup complete!");
}

void StandaloneMode::process() {
    // mDNS handles requests automatically in background
    
    // Check for new lap data - only record during active race
    if (_raceActive && _timingCore && _timingCore->hasNewLap()) {
        LapData lap = _timingCore->getNextLap();
        
        // Grace period: Don't count laps in first 3 seconds after race start
        uint32_t time_since_start = millis() - _raceStartTime;
        if (time_since_start < 3000) {
            Serial.printf("Lap ignored (grace period): %dms after start\n", time_since_start);
            return;  // Skip this lap
        }
        
        // Store lap in our internal list (simple vector)
        _laps.push_back(lap);
        
        // Limit stored laps to prevent memory issues
        if (_laps.size() > 100) {
            _laps.erase(_laps.begin());
        }
        
        Serial.printf("Lap recorded: %dms, RSSI: %d\n", lap.timestamp_ms, lap.rssi_peak);
        
#if ENABLE_LCD_UI
        // Update LCD lap count
        if (_lcdUI) {
            _lcdUI->updateLapCount(_laps.size());
        }
        
#if ENABLE_AUDIO
        // Calculate lap time (difference from previous lap or start)
        uint32_t lapTime = 0;
        if (_laps.size() == 1) {
            lapTime = lap.timestamp_ms - _raceStartTime;
        } else {
            lapTime = lap.timestamp_ms - _laps[_laps.size() - 2].timestamp_ms;
        }
        // Speak lap announcement with time
        speakLapAnnouncement(_laps.size(), lapTime);
#endif
#endif
    }
    
#if ENABLE_LCD_UI
        // Update LCD RSSI (every loop iteration for real-time display)
        if (_lcdUI && _timingCore) {
            _lcdUI->updateRSSI(_timingCore->getCurrentRSSI());
            
            // Update settings display periodically (every 100ms is enough)
            static unsigned long last_settings_update = 0;
            if (millis() - last_settings_update > 100) {
                uint8_t band, channel;
                _timingCore->getRX5808Settings(band, channel);
                _lcdUI->updateBandChannel(band, channel);
                _lcdUI->updateFrequency(_timingCore->getCurrentFrequency());
                
                // Update threshold display (should always match config)
                uint8_t threshold = _timingCore->getThreshold();
                _lcdUI->updateThreshold(threshold);
                
                last_settings_update = millis();
            }
        
#if ENABLE_BATTERY_MONITOR && defined(BATTERY_ADC_PIN)
        // Update battery status every 5 seconds
        static unsigned long last_battery_update = 0;
        if (millis() - last_battery_update > 5000) {
            float voltage = readBatteryVoltage();
            uint8_t percentage = calculateBatteryPercentage(voltage);
            
#if defined(USB_DETECT_PIN)
            bool isCharging = detectUSBConnection();
            _lcdUI->updateBattery(voltage, percentage, isCharging);
#else
            _lcdUI->updateBattery(voltage, percentage);
#endif
            last_battery_update = millis();
        }
#endif
    }
#endif
}

void StandaloneMode::setupWiFiAP() {
    Serial.println("=== Starting WiFi AP Setup ===");
    
    // WiFi was pre-initialized in main.cpp BEFORE timing task starts.
    // This prevents the high-priority timing task from starving WiFi init.
    // Now we reconfigure it with proper SSID, IP, and settings.
    
    // Create unique SSID with MAC address
    // Use softAPmacAddress() for AP mode, not macAddress() (which is for STA mode)
    String macAddr = WiFi.softAPmacAddress();
    
    Serial.printf("AP MAC Address: %s\n", macAddr.c_str());
    
    if (macAddr.length() == 0 || macAddr == "00:00:00:00:00:00") {
        _apSSID = String(WIFI_AP_SSID_PREFIX) + "-ESP32";
    } else {
        macAddr.replace(":", "");
        _apSSID = String(WIFI_AP_SSID_PREFIX) + "-" + macAddr.substring(8);
    }
    
    // For AP mode we want maximum stability, not power saving
    WiFi.setSleep(false);

    // Configure AP IP settings
    WiFi.softAPConfig(
        IPAddress(192, 168, 4, 1),
        IPAddress(192, 168, 4, 1),
        IPAddress(255, 255, 255, 0)
    );

    //WiFi.softAPConfig(powerSaveMode(none));

    // Start AP with configured settings
    bool ap_started = WiFi.softAP(_apSSID.c_str(), WIFI_AP_PASSWORD, 1, 0, 4);
    
    if (ap_started) {
        Serial.println("=== WiFi AP Started ===");
        Serial.printf("SSID: %s\n", _apSSID.c_str());
        Serial.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());
        
        // Set WiFi protocol AFTER AP is started
        esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11N);

    } else {
        Serial.println("ERROR: WiFi AP failed to start");
    }
}


void StandaloneMode::handleRoot() {
    // Serve the static index.html file from SPIFFS
    _server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    _server.sendHeader("Pragma", "no-cache");
    _server.sendHeader("Expires", "0");
    
    // Check if SPIFFS is mounted
    if (!SPIFFS.exists("/index.html")) {
        Serial.println("ERROR: /index.html does not exist in SPIFFS");
        Serial.println("SPIFFS filesystem may be empty or corrupted");
        _server.send(404, "text/plain", "index.html not found in SPIFFS");
        return;
    }
    
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
        Serial.println("ERROR: Failed to open /index.html (file exists but cannot be opened)");
        _server.send(500, "text/plain", "Failed to read index.html");
        return;
    }
    
    if (file.size() == 0) {
        Serial.println("ERROR: /index.html exists but is empty (0 bytes)");
        file.close();
        _server.send(500, "text/plain", "index.html is empty");
        return;
    }
    
    Serial.println("Serving index.html from SPIFFS");
    Serial.printf("File size: %d bytes\n", file.size());
    _server.streamFile(file, "text/html");
    file.close();
}

void StandaloneMode::handleGetStatus() {
    uint8_t current_rssi = _timingCore ? _timingCore->getCurrentRSSI() : 0;
    uint16_t frequency = _timingCore ? _timingCore->getState().frequency_mhz : 5800;
    uint8_t threshold = _timingCore ? _timingCore->getState().threshold : 50;
    bool crossing = _timingCore ? _timingCore->isCrossing() : false;
    
    // Debug output to see what values we're getting
    Serial.printf("[API] RSSI: %d, Freq: %d, Threshold: %d, Crossing: %s\n", 
                  current_rssi, frequency, threshold, crossing ? "true" : "false");
    
    
    String json = "{";
    json += "\"status\":\"" + String(_raceActive ? "racing" : "ready") + "\",";
    json += "\"lap_count\":" + String(_laps.size()) + ",";
    json += "\"uptime\":" + String(millis()) + ",";
    json += "\"rssi\":" + String(current_rssi) + ",";
    json += "\"frequency\":" + String(frequency) + ",";
    json += "\"threshold\":" + String(threshold) + ",";
    json += "\"crossing\":" + String(crossing ? "true" : "false");
    json += "}";
    
    Serial.printf("[API] JSON Response: %s\n", json.c_str());
    _server.send(200, "application/json", json);
}

void StandaloneMode::handleGetLaps() {
    String json = "[";
    
    for (size_t i = 0; i < _laps.size(); i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"lap_number\":" + String(i + 1) + ",";
        json += "\"timestamp_ms\":" + String(_laps[i].timestamp_ms) + ",";
        json += "\"peak_rssi\":" + String(_laps[i].rssi_peak) + ",";
        
        // Calculate lap time (difference from previous lap or start)
        uint32_t lapTime = 0;
        if (i == 0) {
            lapTime = _laps[i].timestamp_ms - _raceStartTime;
        } else {
            lapTime = _laps[i].timestamp_ms - _laps[i-1].timestamp_ms;
        }
        json += "\"lap_time_ms\":" + String(lapTime);
        json += "}";
    }
    
    json += "]";
    _server.send(200, "application/json", json);
}

void StandaloneMode::handleStartRace() {
    _raceActive = true;
    _raceStartTime = millis();
    _laps.clear();
    
    // Drain any pending laps from timing core that may have been detected before race start
    // This ensures the first lap after race start is counted as lap 1
    if (_timingCore) {
        while (_timingCore->hasNewLap()) {
            _timingCore->getNextLap();  // Discard stale laps
        }
    }
    
#if ENABLE_LCD_UI
    if (_lcdUI) {
        _lcdUI->updateRaceStatus(true);
        _lcdUI->updateLapCount(0);
    }
#endif
    
    Serial.println("Race started!");
    _server.send(200, "application/json", "{\"status\":\"race_started\"}");
}

void StandaloneMode::handleStopRace() {
    _raceActive = false;
    
#if ENABLE_LCD_UI
    if (_lcdUI) {
        _lcdUI->updateRaceStatus(false);
    }
#endif
    
    Serial.println("Race stopped!");
    _server.send(200, "application/json", "{\"status\":\"race_stopped\"}");
}

void StandaloneMode::handleClearLaps() {
    _laps.clear();
    
#if ENABLE_LCD_UI
    if (_lcdUI) {
        _lcdUI->updateLapCount(0);
    }
#endif
    
    Serial.println("Laps cleared!");
    _server.send(200, "application/json", "{\"status\":\"laps_cleared\"}");
}

void StandaloneMode::handleStyleCSS() {
    // Try to load from SPIFFS first
    _server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    _server.sendHeader("Pragma", "no-cache");
    _server.sendHeader("Expires", "0");
    
    File file = SPIFFS.open("/style.css", "r");
    if (file && file.size() > 0) {
        Serial.println("Serving style.css from SPIFFS");
        Serial.printf("File size: %d bytes\n", file.size());
        _server.streamFile(file, "text/css");
        file.close();
        return;
    }
    
    // SPIFFS file not found - send minimal error
    Serial.println("ERROR: style.css not found in SPIFFS!");
    Serial.println("Please run 'pio run -t uploadfs' to upload web files");
    
    _server.send(200, "text/css", 
        "body{font-family:Arial,sans-serif;background:#1a1f35;color:#fff;padding:40px;text-align:center;}"
        "h1{color:#ff7b00;margin-bottom:20px;}"
        ".error{background:#2a0f0f;border:2px solid #ff3838;border-radius:8px;padding:30px;max-width:600px;margin:0 auto;}"
    );
}

void StandaloneMode::handleAppJS() {
    // Try to load from SPIFFS first
    _server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    _server.sendHeader("Pragma", "no-cache");
    _server.sendHeader("Expires", "0");
    
    File file = SPIFFS.open("/app.js", "r");
    if (file && file.size() > 0) {
        Serial.println("Serving app.js from SPIFFS");
        Serial.printf("File size: %d bytes\n", file.size());
        _server.streamFile(file, "application/javascript");
        file.close();
        return;
    }
    
    // SPIFFS file not found - send error message to console
    Serial.println("ERROR: app.js not found in SPIFFS!");
    Serial.println("Please run 'pio run -t uploadfs' to upload web files");
    
    _server.send(200, "application/javascript",
        "console.error('app.js not found in SPIFFS - Please upload filesystem');"
        "document.body.innerHTML='<div class=\"error\"><h1>⚠️ Files Missing</h1>"
        "<p>Web interface files not found on device.</p>"
        "<p>Please run: <code>pio run -t uploadfs</code></p></div>';"
    );
}

void StandaloneMode::handleNotFound() {
    _server.send(404, "text/plain", "File not found");
}

void StandaloneMode::handleSetFrequency() {
    if (_server.hasArg("frequency")) {
        int freq = _server.arg("frequency").toInt();
        if (freq >= 5645 && freq <= 5945) {
            if (_timingCore) {
                _timingCore->setFrequency(freq);
                saveSettings();  // Save settings to flash
            }
            _server.send(200, "application/json", "{\"status\":\"frequency_set\",\"frequency\":" + String(freq) + "}");
            Serial.printf("Frequency set to: %d MHz (saved)\n", freq);
        } else {
            _server.send(400, "application/json", "{\"error\":\"invalid_frequency\"}");
        }
    } else {
        _server.send(400, "application/json", "{\"error\":\"missing_frequency\"}");
    }
}

void StandaloneMode::handleSetThreshold() {
    if (_server.hasArg("threshold")) {
        int threshold = _server.arg("threshold").toInt();
        if (threshold >= 0 && threshold <= 255) {
            if (_timingCore) {
                _timingCore->setThreshold(threshold);
                saveSettings();  // Save settings to flash
            }
            _server.send(200, "application/json", "{\"status\":\"threshold_set\",\"threshold\":" + String(threshold) + "}");
            Serial.printf("Threshold set to: %d (saved)\n", threshold);
        } else {
            _server.send(400, "application/json", "{\"error\":\"invalid_threshold\"}");
        }
    } else {
        _server.send(400, "application/json", "{\"error\":\"missing_threshold\"}");
    }
}

void StandaloneMode::handleGetChannels() {
    String json = "{";
    json += "\"bands\":{";
    
    // Raceband (R1-R8)
    json += "\"Raceband\":[";
    int raceband[] = {5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917};
    for (int i = 0; i < 8; i++) {
        if (i > 0) json += ",";
        json += "{\"channel\":\"R" + String(i+1) + "\",\"frequency\":" + String(raceband[i]) + "}";
    }
    json += "],";
    
    // Fatshark (F1-F8)
    json += "\"Fatshark\":[";
    int fatshark[] = {5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880};
    for (int i = 0; i < 8; i++) {
        if (i > 0) json += ",";
        json += "{\"channel\":\"F" + String(i+1) + "\",\"frequency\":" + String(fatshark[i]) + "}";
    }
    json += "],";
    
    // Boscam A (A1-A8)
    json += "\"Boscam_A\":[";
    int boscam_a[] = {5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725};
    for (int i = 0; i < 8; i++) {
        if (i > 0) json += ",";
        json += "{\"channel\":\"A" + String(i+1) + "\",\"frequency\":" + String(boscam_a[i]) + "}";
    }
    json += "],";
    
    // Boscam E (E1-E8)
    json += "\"Boscam_E\":[";
    int boscam_e[] = {5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945};
    for (int i = 0; i < 8; i++) {
        if (i > 0) json += ",";
        json += "{\"channel\":\"E" + String(i+1) + "\",\"frequency\":" + String(boscam_e[i]) + "}";
    }
    json += "]";
    
    json += "}}";
    
    _server.send(200, "application/json", json);
}

void StandaloneMode::handleGetSPIFFSInfo() {
    // Debug endpoint to list SPIFFS files and partition info
    String json = "{";
    json += "\"mounted\":";
    json += SPIFFS.begin(false) ? "true" : "false";
    json += ",";
    
    if (SPIFFS.begin(false)) {
        size_t totalBytes = SPIFFS.totalBytes();
        size_t usedBytes = SPIFFS.usedBytes();
        json += "\"total_bytes\":" + String(totalBytes) + ",";
        json += "\"used_bytes\":" + String(usedBytes) + ",";
        json += "\"free_bytes\":" + String(totalBytes - usedBytes) + ",";
        
        // List files
        json += "\"files\":[";
        File root = SPIFFS.open("/");
        if (root && root.isDirectory()) {
            File file = root.openNextFile();
            bool first = true;
            while (file) {
                if (!first) json += ",";
                json += "{";
                json += "\"name\":\"" + String(file.name()) + "\",";
                json += "\"size\":" + String(file.size());
                json += "}";
                first = false;
                file = root.openNextFile();
            }
        }
        json += "]";
    } else {
        json += "\"error\":\"SPIFFS not mounted\"";
    }
    
    json += "}";
    _server.send(200, "application/json", json);
}

// Web server task - runs at WEB_PRIORITY (2) - below timing (3)
void StandaloneMode::webServerTask(void* parameter) {
    // This task must never return or FreeRTOS will abort.
    // It simply services HTTP requests in a loop.
    StandaloneMode* self = static_cast<StandaloneMode*>(parameter);
    if (!self) {
        // Nothing we can do – delete this task to avoid aborting
        vTaskDelete(nullptr);
        return;
    }

    for (;;) {
        self->_server.handleClient();
        // Yield to other tasks; 5–10ms is enough for responsive HTTP
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

#if defined(BOARD_NUCLEARCOUNTER)
void StandaloneMode::initNuclearDisplay() {
    // 1.3" I2C OLED, SH1106 128x64.
    // Use hardware I2C via U8g2, explicitly binding SCL=9, SDA=8.
    static U8G2_SH1106_128X64_NONAME_F_HW_I2C ncDisplay(
        U8G2_R0,
        /* reset=*/ U8X8_PIN_NONE,
        /* clock=*/ 9,
        /* data=*/ 8
    );

    ncDisplay.begin();
    ncDisplay.clearBuffer();

    // Use a small, readable font
    ncDisplay.setFont(u8g2_font_6x10_tf);

    // Line 1: Device name
    ncDisplay.drawStr(0, 10, "NuclearCounter SFOS");

    // Line 2: IP address (AP IP, usually 192.168.4.1)
    char ipLine[24];
    snprintf(ipLine, sizeof(ipLine), "IP: %s", WiFi.softAPIP().toString().c_str());
    ncDisplay.drawStr(0, 22, ipLine);

    // Line 3: SSID
    char ssidLine[24];
    snprintf(ssidLine, sizeof(ssidLine), "SSID: %s", _apSSID.c_str());
    ncDisplay.drawStr(0, 34, ssidLine);

    // Line 4: Status line (placeholder for now)
    ncDisplay.drawStr(0, 46, "Status: READY");

    ncDisplay.sendBuffer();
}
#endif

#if ENABLE_LCD_UI

#if ENABLE_BATTERY_MONITOR && defined(BATTERY_ADC_PIN)
// Battery voltage monitoring with voltage divider on ADC1 pin
float StandaloneMode::readBatteryVoltage() {
    // Read ADC value (averaging for stability)
    uint32_t adc_sum = 0;
    uint8_t successful_reads = 0;
    
    for (int i = 0; i < BATTERY_SAMPLES; i++) {
        int adc_value = analogRead(g_battery_adc_pin);
        if (adc_value >= 0) {  // Valid ADC reading
            adc_sum += adc_value;
            successful_reads++;
        }
        delay(1);
    }
    
    if (successful_reads == 0) {
        return 0.0;  // Failed to read
    }
    
    uint16_t adc_value = adc_sum / successful_reads;
    
    // Convert ADC reading to voltage
    // With ADC_11db attenuation: 0-4095 maps to approximately 0-3.6V (not 3.3V!)
    // ESP32 ADC is non-linear, but for basic monitoring, use empirical calibration
    // More accurate than assuming 3.3V reference
    float adc_voltage = (adc_value / 4095.0) * 3.6;  // ADC_11db gives ~3.6V range
    float battery_voltage = adc_voltage * BATTERY_VOLTAGE_DIVIDER * BATTERY_ADC_CALIBRATION;
    
    return battery_voltage;
}

uint8_t StandaloneMode::calculateBatteryPercentage(float voltage) {
    // Simple linear mapping (good enough for most LiPo cells)
    // For better accuracy, use a LiPo discharge curve lookup table
    if (voltage >= BATTERY_MAX_VOLTAGE) return 100;
    if (voltage <= BATTERY_MIN_VOLTAGE) return 0;
    
    float percentage = ((voltage - BATTERY_MIN_VOLTAGE) / (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE)) * 100.0;
    return (uint8_t)percentage;
}
#endif

#if defined(USB_DETECT_PIN)
// USB detection via D+ line monitoring
// D+ is normally HIGH (~3.3V) when USB is connected, LOW when disconnected
// Sample multiple times to avoid false readings from USB data traffic
bool StandaloneMode::detectUSBConnection() {
    int highCount = 0;
    
    // Take multiple samples to filter out data pulses
    for (int i = 0; i < USB_DETECT_SAMPLES; i++) {
        if (digitalRead(g_usb_detect_pin) == HIGH) {
            highCount++;
        }
        delayMicroseconds(100);  // Small delay between samples to avoid catching same pulse
    }
    
    // Majority vote: If more than half the samples are HIGH, USB is connected
    // This filters out transient data pulses while reliably detecting USB presence
    return (highCount > (USB_DETECT_SAMPLES / 2));
}
#endif

#if ENABLE_AUDIO
// Initialize audio DAC
void StandaloneMode::initAudio() {
    // Set up GPIO26 as DAC output
    pinMode(AUDIO_DAC_PIN, OUTPUT);
    dacWrite(AUDIO_DAC_PIN, 0);  // Start silent
}

// Play a simple beep for lap detection
void StandaloneMode::playLapBeep() {
    // Generate a 1kHz tone for BEEP_DURATION_MS
    const int frequency = 1000;  // 1kHz
    const int samples = (frequency * BEEP_DURATION_MS) / 1000;
    const float period = 1000000.0 / frequency;  // Period in microseconds
    
    unsigned long start = micros();
    for (int i = 0; i < samples; i++) {
        // Generate square wave (simple but effective)
        int phase = (int)((micros() - start) / (period / 2)) % 2;
        dacWrite(AUDIO_DAC_PIN, phase ? 200 : 55);  // DAC range 0-255, keep volume moderate
        delayMicroseconds(period / 2);
    }
    
    dacWrite(AUDIO_DAC_PIN, 0);  // Silence
}

// Speak lap announcement using word fragment TTS
// NOTE: Only announces lap number and time - NO comparisons to previous laps
// (No "faster lap" or "slower lap" announcements)
void StandaloneMode::speakLapAnnouncement(uint16_t lapNumber, uint32_t lapTimeMs) {
    // Calculate minutes and seconds from lap time
    uint32_t totalSeconds = lapTimeMs / 1000;
    uint32_t minutes = totalSeconds / 60;
    uint32_t seconds = totalSeconds % 60;
    
    #if defined(BOARD_JC2432W328C)
    // TODO: Implement SimpleTTS with pre-recorded word fragments
    // For now, use simple beep until audio files are generated
    // Audio files needed: "lap", "1"-"20", "0"-"59", "seconds", "minutes"
    // Format: Only announce lap number and time (no faster/slower comparisons)
    // Example: "Lap 1, 45 seconds" or "Lap 2, 1 minute, 23 seconds"
    // DO NOT include: "faster lap", "slower lap", "personal best", etc.
    if (minutes > 0) {
        // TODO: Play "lap", lap number, minutes, "minutes", seconds, "seconds"
        // NO comparison to previous lap times
    } else {
        // TODO: Play "lap", lap number, seconds, "seconds" (skip "0 minutes")
        // NO comparison to previous lap times
    }
    playLapBeep();
    #else
    // Fallback to beep for boards without TTS
    playLapBeep();
    #endif
}
#endif

// LCD button callbacks (static, called from LCD UI task)
void StandaloneMode::lcdStartCallback() {
    if (_lcdInstance) {
        _lcdInstance->_raceActive = true;
        _lcdInstance->_raceStartTime = millis();
        _lcdInstance->_laps.clear();
        
        // Drain any pending laps from timing core that may have been detected before race start
        // This ensures the first lap after race start is counted as lap 1
        if (_lcdInstance->_timingCore) {
            while (_lcdInstance->_timingCore->hasNewLap()) {
                _lcdInstance->_timingCore->getNextLap();  // Discard stale laps
            }
        }
        
        if (_lcdInstance->_lcdUI) {
            _lcdInstance->_lcdUI->updateRaceStatus(true);
            _lcdInstance->_lcdUI->updateLapCount(0);
        }
        
        Serial.println("[LCD] Race started!");
    }
}

void StandaloneMode::lcdStopCallback() {
    if (_lcdInstance) {
        _lcdInstance->_raceActive = false;
        
        if (_lcdInstance->_lcdUI) {
            _lcdInstance->_lcdUI->updateRaceStatus(false);
        }
        
        Serial.println("[LCD] Race stopped!");
    }
}

void StandaloneMode::lcdClearCallback() {
    if (_lcdInstance) {
        _lcdInstance->_laps.clear();
        
        if (_lcdInstance->_lcdUI) {
            _lcdInstance->_lcdUI->updateLapCount(0);
        }
        
        Serial.println("[LCD] Laps cleared!");
    }
}
#endif

// Settings persistence using ESP32 Preferences (like EEPROM on Arduino)
void StandaloneMode::loadSettings() {
    Preferences prefs;
    
    // Open preferences in read-only mode
    if (!prefs.begin("sfos", true)) {
        Serial.println("Failed to open preferences for reading");
        return;
    }
    
    // Check if settings exist (using a magic number to verify initialization)
    uint32_t magic = prefs.getUInt("magic", 0);
    if (magic != 0x53464F53) {  // "SFOS" in hex
        Serial.println("No saved settings found, using defaults");
        prefs.end();
        return;
    }
    
    // Load settings
    uint8_t band = prefs.getUChar("band", 0);
    uint8_t channel = prefs.getUChar("channel", 0);
    uint8_t threshold = prefs.getUChar("threshold", CROSSING_THRESHOLD);
    
    prefs.end();
    
    // Apply loaded settings to timing core
    if (_timingCore) {
        _timingCore->setRX5808Settings(band, channel);
        _timingCore->setThreshold(threshold);
        
        Serial.println("\n=== Loaded Settings from Flash ===");
        Serial.printf("Band: %d, Channel: %d\n", band, channel + 1);
        Serial.printf("Frequency: %d MHz\n", _timingCore->getCurrentFrequency());
        Serial.printf("Threshold: %d\n", threshold);
        Serial.println("===================================\n");
    }
}

void StandaloneMode::saveSettings() {
    if (!_timingCore) return;
    
    Preferences prefs;
    
    // Open preferences in read-write mode
    if (!prefs.begin("sfos", false)) {
        Serial.println("Failed to open preferences for writing");
        return;
    }
    
    // Get current settings from timing core
    uint8_t band, channel;
    _timingCore->getRX5808Settings(band, channel);
    uint8_t threshold = _timingCore->getThreshold();
    
    // Save magic number to indicate settings are valid
    prefs.putUInt("magic", 0x53464F53);  // "SFOS" in hex
    
    // Save settings
    prefs.putUChar("band", band);
    prefs.putUChar("channel", channel);
    prefs.putUChar("threshold", threshold);
    
    prefs.end();
    
    Serial.println("Settings saved to flash");
}
