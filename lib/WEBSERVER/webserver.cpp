#include "webserver.h"
#include <ElegantOTA.h>

#include <DNSServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <esp_wifi.h>

#include "debug.h"

#ifdef ESP32S3
#include "rgbled.h"
extern RgbLed* g_rgbLed;
#include <SD.h>
#endif

// Global storage pointer for static functions
static Storage* g_storage = nullptr;

static const uint8_t DNS_PORT = 53;
static IPAddress netMsk(255, 255, 255, 0);
static DNSServer dnsServer;
static IPAddress ipAddress;
static AsyncWebServer server(80);
static AsyncEventSource events("/events");

static const char *wifi_hostname = "FPVGate";
static const char *wifi_ap_ssid_prefix = "FPVGate";
static const char *wifi_ap_password = "fpvgate1";
static const char *wifi_ap_address = "192.168.4.1";
String wifi_ap_ssid;

void Webserver::init(Config *config, LapTimer *lapTimer, BatteryMonitor *batMonitor, Buzzer *buzzer, Led *l, RaceHistory *raceHist, Storage *stor, SelfTest *test, RX5808 *rx5808) {

    ipAddress.fromString(wifi_ap_address);

    conf = config;
    timer = lapTimer;
    monitor = batMonitor;
    buz = buzzer;
    led = l;
    history = raceHist;
    storage = stor;
    g_storage = stor;  // Set global pointer for static functions
    selftest = test;
    rx = rx5808;
    transportMgr = nullptr;

    wifi_ap_ssid = String(wifi_ap_ssid_prefix) + "_" + WiFi.macAddress().substring(WiFi.macAddress().length() - 6);
    wifi_ap_ssid.replace(":", "");
    DEBUG("WiFi AP SSID configured: %s\n", wifi_ap_ssid.c_str());

    WiFi.persistent(false);
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
    esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_LR);
    if (conf->getSsid()[0] == 0) {
        changeMode = WIFI_AP;
    } else {
        changeMode = WIFI_STA;
    }
    changeTimeMs = millis();
    lastStatus = WL_DISCONNECTED;
}

void Webserver::setTransportManager(TransportManager *tm) {
    transportMgr = tm;
}

// TransportInterface implementation
void Webserver::sendLapEvent(uint32_t lapTimeMs) {
    if (!servicesStarted) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", lapTimeMs);
    events.send(buf, "lap");
}

void Webserver::sendRssiEvent(uint8_t rssi) {
    if (!servicesStarted) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", rssi);
    events.send(buf, "rssi");
}

void Webserver::sendRaceStateEvent(const char* state) {
    if (!servicesStarted) return;
    events.send(state, "raceState");
}

bool Webserver::isConnected() {
    // WiFi transport is always "connected" if services are started
    // Individual clients connect/disconnect via SSE but that's transparent
    return servicesStarted;
}

void Webserver::update(uint32_t currentTimeMs) {
    handleWebUpdate(currentTimeMs);
}

void Webserver::handleWebUpdate(uint32_t currentTimeMs) {
    // Note: Lap events are now broadcast via TransportManager in main.cpp
    // This method only handles WiFi-specific logic

    if (sendRssi && ((currentTimeMs - rssiSentMs) > WEB_RSSI_SEND_TIMEOUT_MS)) {
        sendRssiEvent(timer->getRssi());
        rssiSentMs = currentTimeMs;
    }

    // Send SSE keepalive ping to prevent connection timeout
    if (servicesStarted && ((currentTimeMs - sseKeepaliveMs) > WEB_SSE_KEEPALIVE_MS)) {
        events.send("ping", "keepalive", millis());
        sseKeepaliveMs = currentTimeMs;
    }

    wl_status_t status = WiFi.status();

    if (status != lastStatus && wifiMode == WIFI_STA) {
        DEBUG("WiFi status = %u\n", status);
        switch (status) {
            case WL_NO_SSID_AVAIL:
            case WL_CONNECT_FAILED:
            case WL_CONNECTION_LOST:
                changeTimeMs = currentTimeMs;
                changeMode = WIFI_AP;
                break;
            case WL_DISCONNECTED:  // try reconnection
                changeTimeMs = currentTimeMs;
                break;
            case WL_CONNECTED:
                buz->beep(200);
                led->off();
                wifiConnected = true;
                DEBUG("WiFi connected successfully!\n");
                DEBUG("IP address: %s\n", WiFi.localIP().toString().c_str());
                DEBUG("SSID: %s\n", WiFi.SSID().c_str());
#ifdef ESP32S3
                if (g_rgbLed) g_rgbLed->setStatus(STATUS_USER_CONNECTED);
#endif
                break;
            default:
                break;
        }
        lastStatus = status;
    }
    if (status != WL_CONNECTED && wifiMode == WIFI_STA && (currentTimeMs - changeTimeMs) > WIFI_CONNECTION_TIMEOUT_MS) {
        changeTimeMs = currentTimeMs;
        if (!wifiConnected) {
            changeMode = WIFI_AP;  // if we didnt manage to ever connect to wifi network
            // Signal WiFi connection failure with 3 orange LED flashes
#ifdef ESP32S3
            if (g_rgbLed) {
                // Flash orange 3 times (0.5 seconds on, 0.5 seconds off)
                for (int i = 0; i < 3; i++) {
                    g_rgbLed->setColor(CRGB::Orange, RGB_SOLID);
                    delay(500);
                    g_rgbLed->off();
                    delay(500);
                }
            }
#endif
        } else {
            DEBUG("WiFi Connection failed, reconnecting\n");
            WiFi.reconnect();
            startServices();
            buz->beep(100);
            led->blink(200);
        }
    }
    if (changeMode != wifiMode && changeMode != WIFI_OFF && (currentTimeMs - changeTimeMs) > WIFI_RECONNECT_TIMEOUT_MS) {
        switch (changeMode) {
            case WIFI_AP:
                DEBUG("Changing to WiFi AP mode\n");

                WiFi.disconnect();
                wifiMode = WIFI_AP;
                WiFi.setHostname(wifi_hostname);  // hostname must be set before the mode is set to STA
                WiFi.mode(wifiMode);
                changeTimeMs = currentTimeMs;
                WiFi.softAPConfig(ipAddress, ipAddress, netMsk);
                DEBUG("Starting WiFi AP: %s with password: %s\n", wifi_ap_ssid.c_str(), wifi_ap_password);
                WiFi.softAP(wifi_ap_ssid.c_str(), wifi_ap_password);
                DEBUG("WiFi AP started. SSID: %s\n", WiFi.softAPSSID().c_str());
                startServices();
                buz->beep(1000);
                led->on(1000);
                break;
            case WIFI_STA:
                DEBUG("Connecting to WiFi network\n");
                wifiMode = WIFI_STA;
                WiFi.setHostname(wifi_hostname);  // hostname must be set before the mode is set to STA
                WiFi.mode(wifiMode);
                changeTimeMs = currentTimeMs;
                WiFi.begin(conf->getSsid(), conf->getPassword());
                startServices();
                led->blink(200);
            default:
                break;
        }

        changeMode = WIFI_OFF;
    }

    if (servicesStarted) {
        dnsServer.processNextRequest();
    }
}

/** Is this an IP? */
static boolean isIp(String str) {
    for (size_t i = 0; i < str.length(); i++) {
        int c = str.charAt(i);
        if (c != '.' && (c < '0' || c > '9')) {
            return false;
        }
    }
    return true;
}

/** IP to String? */
static String toStringIp(IPAddress ip) {
    String res = "";
    for (int i = 0; i < 3; i++) {
        res += String((ip >> (8 * i)) & 0xFF) + ".";
    }
    res += String(((ip >> 8 * 3)) & 0xFF);
    return res;
}

static bool captivePortal(AsyncWebServerRequest *request) {
    extern const char *wifi_hostname;

    // Allow fpvgate.xyz, IP addresses, and hostname.local without redirecting
    if (!isIp(request->host()) && 
        request->host() != (String(wifi_hostname) + ".local") &&
        request->host() != "fpvgate.xyz" &&
        request->host() != "www.fpvgate.xyz") {
        DEBUG("Request redirected to captive portal\n");
        request->redirect(String("http://") + toStringIp(request->client()->localIP()));
        return true;
    }
    return false;
}

static void handleRoot(AsyncWebServerRequest *request) {
    if (captivePortal(request)) {  // If captive portal redirect instead of displaying the page.
        return;
    }
#ifdef ESP32S3
    // Flash green when user accesses web interface
    extern RgbLed* g_rgbLed;
    if (g_rgbLed) g_rgbLed->flashGreen();
#endif
    request->send(LittleFS, "/index.html", "text/html");
}

static void handleNotFound(AsyncWebServerRequest *request) {
    if (captivePortal(request)) {  // If captive portal redirect instead of displaying the error page.
        return;
    }

    String path = request->url();

#ifdef ESP32S3
    // Try SD card as a fallback for any unknown path (e.g. /sounds_*/file.mp3)
    if (g_storage && g_storage->isSDAvailable() && SD.exists(path)) {
        const char* contentType = "application/octet-stream";
        if (path.endsWith(".mp3")) contentType = "audio/mpeg";
        else if (path.endsWith(".svg")) contentType = "image/svg+xml";
        else if (path.endsWith(".ico")) contentType = "image/x-icon";
        else if (path.endsWith(".json")) contentType = "application/json";
        else if (path.endsWith(".txt")) contentType = "text/plain";
        DEBUG("[404->SD] Serving from SD fallback: %s\n", path.c_str());
        request->send(SD, path, contentType);
        return;
    }
#endif

    String message = F("File Not Found\n\n");
    message += F("URI: ");
    message += path;
    message += F("\nMethod: ");
    message += (request->method() == HTTP_GET) ? "GET" : "POST";
    message += F("\nArguments: ");
    message += request->args();
    message += F("\n");

    for (uint8_t i = 0; i < request->args(); i++) {
        message += String(F(" ")) + request->argName(i) + F(": ") + request->arg(i) + F("\n");
    }
    AsyncWebServerResponse *response = request->beginResponse(404, "text/plain", message);
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "-1");
    request->send(response);
}

static bool startLittleFS() {
    Serial.println("[INFO] Attempting to mount LittleFS...");
    if (!LittleFS.begin(false)) {
        Serial.println("[WARN] LittleFS mount failed, attempting to format...");
        DEBUG("LittleFS mount failed, attempting to format...\n");
        if (!LittleFS.begin(true)) {
            Serial.println("[ERROR] LittleFS format failed!");
            DEBUG("LittleFS format failed\n");
            return false;
        }
        Serial.println("[INFO] LittleFS formatted and mounted");
        DEBUG("LittleFS formatted and mounted\n");
        return true;
    }
    Serial.println("[INFO] LittleFS mounted successfully");
    DEBUG("LittleFS mounted successfully\n");
    return true;
}

static void startMDNS() {
    if (!MDNS.begin(wifi_hostname)) {
        DEBUG("Error starting mDNS\n");
        return;
    }

    String instance = String(wifi_hostname) + "_" + WiFi.macAddress();
    instance.replace(":", "");
    MDNS.setInstanceName(instance);
    MDNS.addService("http", "tcp", 80);
}

void Webserver::startServices() {
    if (servicesStarted) {
        MDNS.end();
        startMDNS();
        return;
    }

    startLittleFS();
    
    // Initialize storage (SD card or LittleFS fallback)
    storage->init();
    
    // Initialize race history after storage is ready
    history->init(storage);

    server.on("/", handleRoot);
    server.on("/generate_204", handleRoot);  // handle Andriod phones doing shit to detect if there is 'real' internet and possibly dropping conn.
    server.on("/gen_204", handleRoot);
    server.on("/library/test/success.html", handleRoot);
    server.on("/hotspot-detect.html", handleRoot);
    server.on("/connectivity-check.html", handleRoot);
    server.on("/check_network_status.txt", handleRoot);
    server.on("/ncsi.txt", handleRoot);
    server.on("/fwlink", handleRoot);

    server.on("/status", [this](AsyncWebServerRequest *request) {
        char buf[1536];
        char configBuf[256];
        conf->toJsonString(configBuf);
        const char *format =
            "\
Heap:\n\
\tFree:\t%i\n\
\tMin:\t%i\n\
\tSize:\t%i\n\
\tAlloc:\t%i\n\
Storage:\n\
\tType:\t%s\n\
\tUsed:\t%llu\n\
\tTotal:\t%llu\n\
\tFree:\t%llu\n\
Chip:\n\
\tModel:\t%s Rev %i, %i Cores, SDK %s\n\
\tFlashSize:\t%i\n\
\tFlashSpeed:\t%iMHz\n\
\tCPU Speed:\t%iMHz\n\
Network:\n\
\tIP:\t%s\n\
\tMAC:\t%s\n\
EEPROM:\n\
%s";

        snprintf(buf, sizeof(buf), format,
                 ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap(),
                 storage->getStorageType().c_str(), storage->getUsedBytes(), storage->getTotalBytes(), storage->getFreeBytes(),
                 ESP.getChipModel(), ESP.getChipRevision(), ESP.getChipCores(), ESP.getSdkVersion(), ESP.getFlashChipSize(), ESP.getFlashChipSpeed() / 1000000, getCpuFrequencyMhz(),
                 WiFi.localIP().toString().c_str(), WiFi.macAddress().c_str(), configBuf);
        request->send(200, "text/plain", buf);
        led->on(200);
    });

    server.on("/timer/start", HTTP_POST, [this](AsyncWebServerRequest *request) {
        timer->start();
        if (transportMgr) {
            transportMgr->broadcastRaceStateEvent("started");
        }
        request->send(200, "application/json", "{\"status\": \"OK\"}");
    });

    server.on("/timer/stop", HTTP_POST, [this](AsyncWebServerRequest *request) {
        timer->stop();
        if (transportMgr) {
            transportMgr->broadcastRaceStateEvent("stopped");
        }
        request->send(200, "application/json", "{\"status\": \"OK\"}");
    });

    server.on("/timer/lap", HTTP_POST, [this](AsyncWebServerRequest *request) {
#ifdef ESP32S3
        if (g_rgbLed) {
            g_rgbLed->flashLap();
        }
#endif
        request->send(200, "application/json", "{\"status\": \"OK\"}");
    });

    // Manual lap addition - broadcasts lap event to all clients
    AsyncCallbackJsonWebHandler *addLapHandler = new AsyncCallbackJsonWebHandler("/timer/addLap", [this](AsyncWebServerRequest *request, JsonVariant &json) {
        JsonObject jsonObj = json.as<JsonObject>();
        if (jsonObj.containsKey("lapTime")) {
            uint32_t lapTimeMs = jsonObj["lapTime"].as<uint32_t>();
            if (transportMgr) {
                transportMgr->broadcastLapEvent(lapTimeMs);
            }
#ifdef ESP32S3
            if (g_rgbLed) {
                g_rgbLed->flashLap();
            }
#endif
        }
        request->send(200, "application/json", "{\"status\": \"OK\"}");
    });
    server.addHandler(addLapHandler);

    server.on("/timer/rssiStart", HTTP_POST, [this](AsyncWebServerRequest *request) {
        sendRssi = true;
        request->send(200, "application/json", "{\"status\": \"OK\"}");
        led->on(200);
    });

    server.on("/timer/rssiStop", HTTP_POST, [this](AsyncWebServerRequest *request) {
        sendRssi = false;
        request->send(200, "application/json", "{\"status\": \"OK\"}");
        led->on(200);
    });

    server.on("/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        conf->toJson(*response);
        request->send(response);
        led->on(200);
    });

    AsyncCallbackJsonWebHandler *configJsonHandler = new AsyncCallbackJsonWebHandler("/config", [this](AsyncWebServerRequest *request, JsonVariant &json) {
        JsonObject jsonObj = json.as<JsonObject>();
#ifdef DEBUG_OUT
        serializeJsonPretty(jsonObj, DEBUG_OUT);
        DEBUG("\n");
#endif
        conf->fromJson(jsonObj);
        request->send(200, "application/json", "{\"status\": \"OK\"}");
        led->on(200);
    });

    // Serve audio files from SD card voice directories (sounds_default, sounds_rachel, etc.)
    server.on("^\\/sounds_.+\\/.+\\.mp3$", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String path = request->url();
        
#ifdef ESP32S3
        // Try SD card first if available
        if (storage->isSDAvailable() && SD.exists(path)) {
            DEBUG("Serving audio from SD: %s\n", path.c_str());
            request->send(SD, path, "audio/mpeg");
            return;
        }
#endif
        
        // Fall back to LittleFS
        if (LittleFS.exists(path)) {
            DEBUG("Serving audio from LittleFS: %s\n", path.c_str());
            request->send(LittleFS, path, "audio/mpeg");
            return;
        }
        
        // File not found
        DEBUG("Audio file not found: %s\n", path.c_str());
        request->send(404, "text/plain", "Audio file not found");
    });
    
    // Serve audio files from SD /sounds/ directory (legacy/fallback)
    server.on("^\\/sounds\\/.+\\.mp3$", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String path = request->url();
        
#ifdef ESP32S3
        // Try SD card first if available
        if (storage->isSDAvailable() && SD.exists(path)) {
            DEBUG("Serving audio from SD: %s\n", path.c_str());
            request->send(SD, path, "audio/mpeg");
            return;
        }
#endif
        
        // Fall back to LittleFS
        if (LittleFS.exists(path)) {
            DEBUG("Serving audio from LittleFS: %s\n", path.c_str());
            request->send(LittleFS, path, "audio/mpeg");
            return;
        }
        
        // File not found
        DEBUG("Audio file not found: %s\n", path.c_str());
        request->send(404, "text/plain", "Audio file not found");
    });
    
    // WiFi status endpoint (register before serveStatic to prevent VFS errors)
    server.on("/api/wifi", HTTP_GET, [this](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(512);
        
        // Get WiFi mode
        wifi_mode_t mode = WiFi.getMode();
        if (mode == WIFI_AP) {
            doc["mode"] = "AP";
            doc["ssid"] = WiFi.softAPSSID();
            doc["ip"] = WiFi.softAPIP().toString();
            doc["clients"] = WiFi.softAPgetStationNum();
            doc["rssi"] = 0;
        } else if (mode == WIFI_STA) {
            doc["mode"] = "STA";
            doc["ssid"] = WiFi.SSID();
            doc["ip"] = WiFi.localIP().toString();
            doc["clients"] = 0;
            doc["rssi"] = WiFi.RSSI();
            doc["connected"] = WiFi.status() == WL_CONNECTED;
        } else {
            doc["mode"] = "OFF";
            doc["ssid"] = "";
            doc["ip"] = "";
            doc["clients"] = 0;
            doc["rssi"] = 0;
            doc["connected"] = false;
        }
        
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
        led->on(200);
    });
    
    // Serve other static files from LittleFS only
    server.serveStatic("/", LittleFS, "/").setCacheControl("max-age=600");

    events.onConnect([this](AsyncEventSourceClient *client) {
        if (client->lastId()) {
            DEBUG("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
        }
        client->send("start", NULL, millis(), 1000);
        led->on(200);
    });

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Max-Age", "600");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");

    server.onNotFound(handleNotFound);

    server.addHandler(&events);
    server.addHandler(configJsonHandler);

    // Race history endpoints
    server.on("/races", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String json = history->toJsonString();
        request->send(200, "application/json", json);
        led->on(200);
    });

    server.on("/races/download", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String json = history->toJsonString();
        AsyncWebServerResponse *response = request->beginResponse(200, "application/octet-stream", json);
        response->addHeader("Content-Disposition", "attachment; filename=\"races.json\"");
        response->addHeader("Content-Type", "application/json");
        request->send(response);
        led->on(200);
    });

    AsyncCallbackJsonWebHandler *raceSaveHandler = new AsyncCallbackJsonWebHandler("/races/save", [this](AsyncWebServerRequest *request, JsonVariant &json) {
        JsonObject jsonObj = json.as<JsonObject>();
        
        RaceSession race;
        race.timestamp = jsonObj["timestamp"];
        race.fastestLap = jsonObj["fastestLap"];
        race.medianLap = jsonObj["medianLap"];
        race.best3LapsTotal = jsonObj["best3LapsTotal"];
        race.pilotName = jsonObj["pilotName"] | "";
        race.pilotCallsign = jsonObj["pilotCallsign"] | "";
        race.frequency = jsonObj["frequency"] | 0;
        race.band = jsonObj["band"] | "";
        race.channel = jsonObj["channel"] | 0;
        
        JsonArray lapsArray = jsonObj["lapTimes"];
        for (uint32_t lap : lapsArray) {
            race.lapTimes.push_back(lap);
        }
        
        bool success = history->saveRace(race);
        request->send(200, "application/json", success ? "{\"status\": \"OK\"}" : "{\"status\": \"ERROR\"}");
        led->on(200);
    });

    AsyncCallbackJsonWebHandler *raceUploadHandler = new AsyncCallbackJsonWebHandler("/races/upload", [this](AsyncWebServerRequest *request, JsonVariant &json) {
        String jsonString;
        serializeJson(json, jsonString);
        bool success = history->fromJsonString(jsonString);
        request->send(200, "application/json", success ? "{\"status\": \"OK\"}" : "{\"status\": \"ERROR\"}");
        led->on(200);
    });

    server.on("/races/delete", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("timestamp", true)) {
            uint32_t timestamp = request->getParam("timestamp", true)->value().toInt();
            bool success = history->deleteRace(timestamp);
            request->send(200, "application/json", success ? "{\"status\": \"OK\"}" : "{\"status\": \"ERROR\"}");
        } else {
            request->send(400, "application/json", "{\"status\": \"ERROR\", \"message\": \"Missing timestamp\"}");
        }
        led->on(200);
    });

    server.on("/races/clear", HTTP_POST, [this](AsyncWebServerRequest *request) {
        bool success = history->clearAll();
        request->send(200, "application/json", success ? "{\"status\": \"OK\"}" : "{\"status\": \"ERROR\"}");
        led->on(200);
    });

    server.on("/races/update", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("timestamp", true) && 
            request->hasParam("name", true) && 
            request->hasParam("tag", true)) {
            uint32_t timestamp = request->getParam("timestamp", true)->value().toInt();
            String name = request->getParam("name", true)->value();
            String tag = request->getParam("tag", true)->value();
            bool success = history->updateRace(timestamp, name, tag);
            request->send(200, "application/json", success ? "{\"status\": \"OK\"}" : "{\"status\": \"ERROR\"}");
        } else {
            request->send(400, "application/json", "{\"status\": \"ERROR\", \"message\": \"Missing parameters\"}");
        }
        led->on(200);
    });

    AsyncCallbackJsonWebHandler *updateLapsHandler = new AsyncCallbackJsonWebHandler("/races/updateLaps", [this](AsyncWebServerRequest *request, JsonVariant &json) {
        JsonObject jsonObj = json.as<JsonObject>();
        
        if (!jsonObj.containsKey("timestamp") || !jsonObj.containsKey("lapTimes")) {
            request->send(400, "application/json", "{\"status\": \"ERROR\", \"message\": \"Missing parameters\"}");
            return;
        }
        
        uint32_t timestamp = jsonObj["timestamp"];
        JsonArray lapsArray = jsonObj["lapTimes"];
        
        std::vector<uint32_t> lapTimes;
        for (JsonVariant lap : lapsArray) {
            lapTimes.push_back(lap.as<uint32_t>());
        }
        
        bool success = history->updateLaps(timestamp, lapTimes);
        request->send(200, "application/json", success ? "{\"status\": \"OK\"}" : "{\"status\": \"ERROR\"}");
        led->on(200);
    });

    server.on("/races/downloadOne", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("timestamp")) {
            uint32_t timestamp = request->getParam("timestamp")->value().toInt();
            
            // Find the race
            const auto& races = history->getRaces();
            for (const auto& race : races) {
                if (race.timestamp == timestamp) {
                    // Create JSON for single race
                    DynamicJsonDocument doc(16384);
                    JsonArray racesArray = doc.createNestedArray("races");
                    JsonObject raceObj = racesArray.createNestedObject();
                    raceObj["timestamp"] = race.timestamp;
                    raceObj["fastestLap"] = race.fastestLap;
                    raceObj["medianLap"] = race.medianLap;
                    raceObj["best3LapsTotal"] = race.best3LapsTotal;
                    raceObj["name"] = race.name;
                    raceObj["tag"] = race.tag;
                    raceObj["pilotName"] = race.pilotName;
                    raceObj["pilotCallsign"] = race.pilotCallsign;
                    raceObj["frequency"] = race.frequency;
                    raceObj["band"] = race.band;
                    raceObj["channel"] = race.channel;
                    JsonArray lapsArray = raceObj.createNestedArray("lapTimes");
                    for (uint32_t lap : race.lapTimes) {
                        lapsArray.add(lap);
                    }
                    
                    String json;
                    serializeJson(doc, json);
                    
                    String filename = "race_" + String(timestamp) + ".json";
                    AsyncWebServerResponse *response = request->beginResponse(200, "application/octet-stream", json);
                    response->addHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
                    response->addHeader("Content-Type", "application/json");
                    request->send(response);
                    led->on(200);
                    return;
                }
            }
            request->send(404, "application/json", "{\"status\": \"ERROR\", \"message\": \"Race not found\"}");
        } else {
            request->send(400, "application/json", "{\"status\": \"ERROR\", \"message\": \"Missing timestamp\"}");
        }
        led->on(200);
    });

    server.addHandler(raceSaveHandler);
    server.addHandler(raceUploadHandler);
    server.addHandler(updateLapsHandler);

    // Self-test endpoint
    server.on("/selftest", HTTP_GET, [this](AsyncWebServerRequest *request) {
        selftest->runAllTests();
        String json = selftest->getResultsJSON();
        request->send(200, "application/json", json);
        led->on(200);
    });
    
    // Reboot endpoint
    server.on("/reboot", HTTP_POST, [this](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"status\": \"OK\", \"message\": \"Rebooting...\"}");
        led->on(200);
        delay(500);
        ESP.restart();
    });

    // SD card initialization endpoint
    server.on("/storage/initsd", HTTP_POST, [this](AsyncWebServerRequest *request) {
        bool success = storage->initSDDeferred();
        String json = success ? "{\"status\":\"OK\",\"message\":\"SD card initialized\"}" : 
                               "{\"status\":\"ERROR\",\"message\":\"SD card init failed\"}";
        request->send(success ? 200 : 500, "application/json", json);
        led->on(200);
    });
    
    // SD card test endpoint - list files
    server.on("/storage/sdtest", HTTP_GET, [this](AsyncWebServerRequest *request) {
#ifdef ESP32S3
        String response = "SD Card Test:\n\n";
        response += "Available: " + String(storage->isSDAvailable() ? "YES" : "NO") + "\n";
        response += "Storage Type: " + storage->getStorageType() + "\n";
        
        if (storage->isSDAvailable()) {
            response += "\nRoot directories:\n";
            File root = SD.open("/");
            if (root) {
                File entry = root.openNextFile();
                while (entry) {
                    if (entry.isDirectory()) {
                        response += "  [DIR] " + String(entry.name()) + "\n";
                        
                        // List first 5 files in sounds_* directories
                        String entryName = String(entry.name());
                        if (entryName.startsWith("/sounds_")) {
                            File subdir = SD.open(entry.name());
                            if (subdir) {
                                int count = 0;
                                File file = subdir.openNextFile();
                                while (file && count < 5) {
                                    response += "    - " + String(file.name()) + " (" + String(file.size()) + " bytes)\n";
                                    file = subdir.openNextFile();
                                    count++;
                                }
                                if (count == 5) response += "    ... (more files)\n";
                                subdir.close();
                            }
                        }
                    } else {
                        response += "  [FILE] " + String(entry.name()) + " (" + String(entry.size()) + " bytes)\n";
                    }
                    entry = root.openNextFile();
                }
                root.close();
            } else {
                response += "ERROR: Could not open root directory\n";
            }
            
            // Test specific file
            response += "\nTest file access:\n";
            const char* testFile = "/sounds_adam/gate_1.mp3";
            response += "  " + String(testFile) + ": ";
            if (SD.exists(testFile)) {
                File f = SD.open(testFile);
                if (f) {
                    response += "EXISTS, size=" + String(f.size()) + " bytes\n";
                    f.close();
                } else {
                    response += "EXISTS but CANNOT OPEN\n";
                }
            } else {
                response += "NOT FOUND\n";
            }
        } else {
            response += "\nSD card not available!\n";
        }
        
        request->send(200, "text/plain", response);
#else
        request->send(200, "text/plain", "SD card not supported on this platform");
#endif
        led->on(200);
    });

#ifdef ESP32S3
    // LED control endpoints
    server.on("/led/color", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("color", true)) {
            String colorStr = request->getParam("color", true)->value();
            uint32_t color = strtol(colorStr.c_str(), NULL, 16);
            if (g_rgbLed) {
                g_rgbLed->setManualColor(color);
            }
            conf->setLedColor(color);
            request->send(200, "application/json", "{\"status\": \"OK\"}");
        } else {
            request->send(400, "application/json", "{\"status\": \"ERROR\", \"message\": \"Missing color\"}");
        }
        led->on(200);
    });

    server.on("/led/mode", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("mode", true)) {
            uint8_t mode = request->getParam("mode", true)->value().toInt();
            if (g_rgbLed) {
                if (mode == 0) {
                    g_rgbLed->off();
                } else if (mode == 1) {
                    g_rgbLed->setManualMode(RGB_SOLID);
                } else if (mode == 2) {
                    g_rgbLed->setManualMode(RGB_PULSE);
                } else if (mode == 3) {
                    g_rgbLed->setRainbowWave();
                }
            }
            request->send(200, "application/json", "{\"status\": \"OK\"}");
        } else {
            request->send(400, "application/json", "{\"status\": \"ERROR\", \"message\": \"Missing mode\"}");
        }
        led->on(200);
    });

    server.on("/led/brightness", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("brightness", true)) {
            uint8_t brightness = request->getParam("brightness", true)->value().toInt();
            if (g_rgbLed) {
                g_rgbLed->setBrightness(brightness);
            }
            conf->setLedBrightness(brightness);
            request->send(200, "application/json", "{\"status\": \"OK\"}");
        } else {
            request->send(400, "application/json", "{\"status\": \"ERROR\", \"message\": \"Missing brightness\"}");
        }
        led->on(200);
    });

    server.on("/led/preset", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("preset", true)) {
            uint8_t preset = request->getParam("preset", true)->value().toInt();
            if (g_rgbLed) {
                g_rgbLed->setPreset((led_preset_e)preset);
            }
            conf->setLedPreset(preset);
            request->send(200, "application/json", "{\"status\": \"OK\"}");
        } else {
            request->send(400, "application/json", "{\"status\": \"ERROR\", \"message\": \"Missing preset\"}");
        }
        led->on(200);
    });

    server.on("/led/override", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("enable", true)) {
            bool enable = request->getParam("enable", true)->value() == "1";
            if (g_rgbLed) {
                g_rgbLed->enableManualOverride(enable);
            }
            conf->setLedManualOverride(enable ? 1 : 0);
            request->send(200, "application/json", "{\"status\": \"OK\"}");
        } else {
            request->send(400, "application/json", "{\"status\": \"ERROR\", \"message\": \"Missing enable\"}");
        }
        led->on(200);
    });

    server.on("/led/error", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("code", true)) {
            uint8_t code = request->getParam("code", true)->value().toInt();
            if (g_rgbLed) {
                g_rgbLed->showErrorCode(code);
            }
            request->send(200, "application/json", "{\"status\": \"OK\"}");
        } else {
            request->send(400, "application/json", "{\"status\": \"ERROR\", \"message\": \"Missing code\"}");
        }
        led->on(200);
    });

    server.on("/led/speed", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("speed", true)) {
            uint8_t speed = request->getParam("speed", true)->value().toInt();
            if (g_rgbLed) {
                g_rgbLed->setEffectSpeed(speed);
            }
            conf->setLedSpeed(speed);
            request->send(200, "application/json", "{\"status\": \"OK\"}");
        } else {
            request->send(400, "application/json", "{\"status\": \"ERROR\", \"message\": \"Missing speed\"}");
        }
        led->on(200);
    });

    server.on("/led/fadecolor", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("color", true)) {
            String colorStr = request->getParam("color", true)->value();
            uint32_t color = strtol(colorStr.c_str(), NULL, 16);
            if (g_rgbLed) {
                g_rgbLed->setFadeColor(color);
            }
            conf->setLedFadeColor(color);
            request->send(200, "application/json", "{\"status\": \"OK\"}");
        } else {
            request->send(400, "application/json", "{\"status\": \"ERROR\", \"message\": \"Missing color\"}");
        }
        led->on(200);
    });

    server.on("/led/strobecolor", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("color", true)) {
            String colorStr = request->getParam("color", true)->value();
            uint32_t color = strtol(colorStr.c_str(), NULL, 16);
            if (g_rgbLed) {
                g_rgbLed->setStrobeColor(color);
            }
            conf->setLedStrobeColor(color);
            request->send(200, "application/json", "{\"status\": \"OK\"}");
        } else {
            request->send(400, "application/json", "{\"status\": \"ERROR\", \"message\": \"Missing color\"}");
        }
        led->on(200);
    });
#endif

    // Calibration wizard endpoints
    server.on("/calibration/start", HTTP_POST, [this](AsyncWebServerRequest *request) {
        timer->startCalibrationWizard();
        request->send(200, "application/json", "{\"status\": \"OK\"}");
        led->on(200);
    });

    server.on("/calibration/stop", HTTP_POST, [this](AsyncWebServerRequest *request) {
        timer->stopCalibrationWizard();
        request->send(200, "application/json", "{\"status\": \"OK\"}");
        led->on(200);
    });

    server.on("/calibration/data", HTTP_GET, [this](AsyncWebServerRequest *request) {
        uint16_t count = timer->getCalibrationRssiCount();
        
        // Build JSON response
        String json = "{\"count\":" + String(count) + ",\"data\":[";
        for (uint16_t i = 0; i < count; i++) {
            if (i > 0) json += ",";
            json += "{\"rssi\":" + String(timer->getCalibrationRssi(i)) + ",\"time\":" + String(timer->getCalibrationTimestamp(i)) + "}";
        }
        json += "]}";
        
        request->send(200, "application/json", json);
        led->on(200);
    });

    // Self-test endpoint
    server.on("/api/selftest", HTTP_GET, [this](AsyncWebServerRequest *request) {
        // Run RX5808 test
        TestResult rxTest = selftest->testRX5808(rx);
        
        // Run Lap Timer test
        TestResult timerTest = selftest->testLapTimer(timer);
        
        // Run Audio test
        TestResult audioTest = selftest->testAudio(buz);
        
        // Run Config test
        TestResult configTest = selftest->testConfig(conf);
        
        // Run Race History test
        TestResult historyTest = selftest->testRaceHistory(history);
        
        // Run Web Server test
        TestResult webTest = selftest->testWebServer();
        
        // Run OTA test
        TestResult otaTest = selftest->testOTA();
        
        // Run Storage test
        TestResult storageTest = selftest->testStorage();
        
        // Run LittleFS test
        TestResult littleFSTest = selftest->testLittleFS();
        
        // Run EEPROM test
        TestResult eepromTest = selftest->testEEPROM();
        
        // Run WiFi test
        TestResult wifiTest = selftest->testWiFi();
        
        // Run Battery test
        TestResult batteryTest = selftest->testBattery();
        
#ifdef ESP32S3
        // Run RGB LED test
        TestResult ledTest = selftest->testRGBLED(g_rgbLed);
#endif
        
        // Build JSON response
        String json = "{\"tests\":[";
        
        auto addTest = [&json](const TestResult& test, bool first = false) {
            if (!first) json += ",";
            json += "{\"name\":\"" + test.name + "\",\"passed\":" + String(test.passed ? "true" : "false") + 
                   ",\"details\":\"" + test.details + "\",\"duration\":" + String(test.duration_ms) + "}";
        };
        
        addTest(rxTest, true);
        addTest(timerTest);
        addTest(audioTest);
        addTest(configTest);
        addTest(historyTest);
        addTest(webTest);
        addTest(otaTest);
        addTest(storageTest);
        addTest(littleFSTest);
        addTest(eepromTest);
        addTest(wifiTest);
        addTest(batteryTest);
#ifdef ESP32S3
        addTest(ledTest);
#endif
        
        json += "]}";
        
        request->send(200, "application/json", json);
        led->on(200);
    });

    ElegantOTA.setAutoReboot(true);
    ElegantOTA.begin(&server);

    server.begin();

    dnsServer.start(DNS_PORT, "*", ipAddress);
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);

    startMDNS();

    servicesStarted = true;
}
