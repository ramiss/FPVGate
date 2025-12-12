#include "config.h"

#include <EEPROM.h>

#include "debug.h"

void Config::init(void) {
    if (sizeof(laptimer_config_t) > EEPROM_RESERVED_SIZE) {
        DEBUG("Config size too big, adjust reserved EEPROM size\n");
        return;
    }

    EEPROM.begin(EEPROM_RESERVED_SIZE);  // Size of EEPROM
    load();                              // Override default settings from EEPROM

    checkTimeMs = millis();

    DEBUG("EEPROM Init Successful\n");
}

void Config::load(void) {
    modified = false;
    EEPROM.get(0, conf);

    uint32_t version = 0xFFFFFFFF;
    if ((conf.version & CONFIG_MAGIC_MASK) == CONFIG_MAGIC) {
        version = conf.version & ~CONFIG_MAGIC_MASK;
    }

    // If version is not current, reset to defaults
    if (version != CONFIG_VERSION) {
        setDefaults();
    }
}

void Config::write(void) {
    if (!modified) return;

    DEBUG("Writing to EEPROM\n");

    EEPROM.put(0, conf);
    EEPROM.commit();

    DEBUG("Writing to EEPROM done\n");

    modified = false;
}

void Config::toJson(AsyncResponseStream& destination) {
    // Use https://arduinojson.org/v6/assistant to estimate memory
    DynamicJsonDocument config(512);
    config["freq"] = conf.frequency;
    config["minLap"] = conf.minLap;
    config["alarm"] = conf.alarm;
    config["anType"] = conf.announcerType;
    config["anRate"] = conf.announcerRate;
    config["enterRssi"] = conf.enterRssi;
    config["exitRssi"] = conf.exitRssi;
    config["maxLaps"] = conf.maxLaps;
    config["ledMode"] = conf.ledMode;
    config["ledBrightness"] = conf.ledBrightness;
    config["ledColor"] = conf.ledColor;
    config["ledPreset"] = conf.ledPreset;
    config["ledSpeed"] = conf.ledSpeed;
    config["ledFadeColor"] = conf.ledFadeColor;
    config["ledStrobeColor"] = conf.ledStrobeColor;
    config["ledManualOverride"] = conf.ledManualOverride;
    config["opMode"] = conf.operationMode;
    config["tracksEnabled"] = conf.tracksEnabled;
    config["selectedTrackId"] = conf.selectedTrackId;
    config["webhooksEnabled"] = conf.webhooksEnabled;
    config["webhookCount"] = conf.webhookCount;
    JsonArray webhooks = config.createNestedArray("webhookIPs");
    for (uint8_t i = 0; i < conf.webhookCount; i++) {
        webhooks.add(conf.webhookIPs[i]);
    }
    config["gateLEDsEnabled"] = conf.gateLEDsEnabled;
    config["webhookRaceStart"] = conf.webhookRaceStart;
    config["webhookRaceStop"] = conf.webhookRaceStop;
    config["webhookLap"] = conf.webhookLap;
    config["name"] = conf.pilotName;
    config["ssid"] = conf.ssid;
    config["pwd"] = conf.password;
    serializeJson(config, destination);
}

void Config::toJsonString(char* buf) {
    DynamicJsonDocument config(512);
    config["freq"] = conf.frequency;
    config["minLap"] = conf.minLap;
    config["alarm"] = conf.alarm;
    config["anType"] = conf.announcerType;
    config["anRate"] = conf.announcerRate;
    config["enterRssi"] = conf.enterRssi;
    config["exitRssi"] = conf.exitRssi;
    config["maxLaps"] = conf.maxLaps;
    config["ledMode"] = conf.ledMode;
    config["ledBrightness"] = conf.ledBrightness;
    config["ledColor"] = conf.ledColor;
    config["ledPreset"] = conf.ledPreset;
    config["ledSpeed"] = conf.ledSpeed;
    config["ledFadeColor"] = conf.ledFadeColor;
    config["ledStrobeColor"] = conf.ledStrobeColor;
    config["ledManualOverride"] = conf.ledManualOverride;
    config["opMode"] = conf.operationMode;
    config["tracksEnabled"] = conf.tracksEnabled;
    config["selectedTrackId"] = conf.selectedTrackId;
    config["name"] = conf.pilotName;
    config["ssid"] = conf.ssid;
    config["pwd"] = conf.password;
    serializeJsonPretty(config, buf, 256);
}

void Config::fromJson(JsonObject source) {
    if (source["freq"] != conf.frequency) {
        conf.frequency = source["freq"];
        modified = true;
    }
    if (source["minLap"] != conf.minLap) {
        conf.minLap = source["minLap"];
        modified = true;
    }
    if (source["alarm"] != conf.alarm) {
        conf.alarm = source["alarm"];
        modified = true;
    }
    if (source["anType"] != conf.announcerType) {
        conf.announcerType = source["anType"];
        modified = true;
    }
    if (source["anRate"] != conf.announcerRate) {
        conf.announcerRate = source["anRate"];
        modified = true;
    }
    if (source["enterRssi"] != conf.enterRssi) {
        conf.enterRssi = source["enterRssi"];
        modified = true;
    }
    if (source["exitRssi"] != conf.exitRssi) {
        conf.exitRssi = source["exitRssi"];
        modified = true;
    }
    if (source["maxLaps"] != conf.maxLaps) {
        conf.maxLaps = source["maxLaps"];
        modified = true;
    }
    if (source.containsKey("ledMode") && source["ledMode"] != conf.ledMode) {
        conf.ledMode = source["ledMode"];
        modified = true;
    }
    if (source.containsKey("ledBrightness") && source["ledBrightness"] != conf.ledBrightness) {
        conf.ledBrightness = source["ledBrightness"];
        modified = true;
    }
    if (source.containsKey("ledColor") && source["ledColor"] != conf.ledColor) {
        conf.ledColor = source["ledColor"];
        modified = true;
    }
    if (source.containsKey("ledPreset") && source["ledPreset"] != conf.ledPreset) {
        conf.ledPreset = source["ledPreset"];
        modified = true;
    }
    if (source.containsKey("ledSpeed") && source["ledSpeed"] != conf.ledSpeed) {
        conf.ledSpeed = source["ledSpeed"];
        modified = true;
    }
    if (source.containsKey("ledFadeColor") && source["ledFadeColor"] != conf.ledFadeColor) {
        conf.ledFadeColor = source["ledFadeColor"];
        modified = true;
    }
    if (source.containsKey("ledStrobeColor") && source["ledStrobeColor"] != conf.ledStrobeColor) {
        conf.ledStrobeColor = source["ledStrobeColor"];
        modified = true;
    }
    if (source.containsKey("ledManualOverride") && source["ledManualOverride"] != conf.ledManualOverride) {
        conf.ledManualOverride = source["ledManualOverride"];
        modified = true;
    }
    if (source.containsKey("opMode") && source["opMode"] != conf.operationMode) {
        conf.operationMode = source["opMode"];
        modified = true;
    }
    if (source.containsKey("tracksEnabled") && source["tracksEnabled"] != conf.tracksEnabled) {
        conf.tracksEnabled = source["tracksEnabled"];
        modified = true;
    }
    if (source.containsKey("selectedTrackId") && source["selectedTrackId"] != conf.selectedTrackId) {
        conf.selectedTrackId = source["selectedTrackId"];
        modified = true;
    }
    if (source.containsKey("gateLEDsEnabled") && source["gateLEDsEnabled"] != conf.gateLEDsEnabled) {
        conf.gateLEDsEnabled = source["gateLEDsEnabled"];
        modified = true;
    }
    if (source.containsKey("webhookRaceStart") && source["webhookRaceStart"] != conf.webhookRaceStart) {
        conf.webhookRaceStart = source["webhookRaceStart"];
        modified = true;
    }
    if (source.containsKey("webhookRaceStop") && source["webhookRaceStop"] != conf.webhookRaceStop) {
        conf.webhookRaceStop = source["webhookRaceStop"];
        modified = true;
    }
    if (source.containsKey("webhookLap") && source["webhookLap"] != conf.webhookLap) {
        conf.webhookLap = source["webhookLap"];
        modified = true;
    }
    if (source["name"] != conf.pilotName) {
        strlcpy(conf.pilotName, source["name"] | "", sizeof(conf.pilotName));
        modified = true;
    }
    if (source["ssid"] != conf.ssid) {
        strlcpy(conf.ssid, source["ssid"] | "", sizeof(conf.ssid));
        modified = true;
    }
    if (source["pwd"] != conf.password) {
        strlcpy(conf.password, source["pwd"] | "", sizeof(conf.password));
        modified = true;
    }
}

uint16_t Config::getFrequency() {
    // === TEMPORARY HARDCODE FOR RX5808 CH1 PIN ISSUE ===
    // Hardcoded to R1 (5658 MHz) - Raceband Channel 1
    // TODO: Remove this once CH1 pin is fixed and revert to: return conf.frequency;
    return 5658;
    // === END TEMPORARY HARDCODE ===
}

uint32_t Config::getMinLapMs() {
    return conf.minLap * 100;
}

uint8_t Config::getAlarmThreshold() {
    return conf.alarm;
}

uint8_t Config::getEnterRssi() {
    return conf.enterRssi;
}

uint8_t Config::getExitRssi() {
    return conf.exitRssi;
}

char* Config::getSsid() {
    return conf.ssid;
}

char* Config::getPassword() {
    return conf.password;
}

uint8_t Config::getMaxLaps() {
    return conf.maxLaps;
}

uint8_t Config::getLedMode() {
    return conf.ledMode;
}

uint8_t Config::getLedBrightness() {
    return conf.ledBrightness;
}

uint32_t Config::getLedColor() {
    return conf.ledColor;
}

uint8_t Config::getLedPreset() {
    return conf.ledPreset;
}

uint8_t Config::getLedSpeed() {
    return conf.ledSpeed;
}

uint32_t Config::getLedFadeColor() {
    return conf.ledFadeColor;
}

uint32_t Config::getLedStrobeColor() {
    return conf.ledStrobeColor;
}

uint8_t Config::getLedManualOverride() {
    return conf.ledManualOverride;
}

uint8_t Config::getOperationMode() {
    return conf.operationMode;
}

uint8_t Config::getTracksEnabled() {
    return conf.tracksEnabled;
}

uint32_t Config::getSelectedTrackId() {
    return conf.selectedTrackId;
}

uint8_t Config::getWebhooksEnabled() {
    return conf.webhooksEnabled;
}

uint8_t Config::getWebhookCount() {
    return conf.webhookCount;
}

const char* Config::getWebhookIP(uint8_t index) {
    if (index < conf.webhookCount) {
        return conf.webhookIPs[index];
    }
    return nullptr;
}

uint8_t Config::getGateLEDsEnabled() {
    return conf.gateLEDsEnabled;
}

uint8_t Config::getWebhookRaceStart() {
    return conf.webhookRaceStart;
}

uint8_t Config::getWebhookRaceStop() {
    return conf.webhookRaceStop;
}

uint8_t Config::getWebhookLap() {
    return conf.webhookLap;
}

// Setters for RotorHazard node mode
void Config::setFrequency(uint16_t freq) {
    if (conf.frequency != freq) {
        conf.frequency = freq;
        modified = true;
    }
}

void Config::setEnterRssi(uint8_t rssi) {
    if (conf.enterRssi != rssi) {
        conf.enterRssi = rssi;
        modified = true;
    }
}

void Config::setExitRssi(uint8_t rssi) {
    if (conf.exitRssi != rssi) {
        conf.exitRssi = rssi;
        modified = true;
    }
}

void Config::setOperationMode(uint8_t mode) {
    if (conf.operationMode != mode) {
        conf.operationMode = mode;
        modified = true;
    }
}

void Config::setLedPreset(uint8_t preset) {
    if (conf.ledPreset != preset) {
        conf.ledPreset = preset;
        modified = true;
    }
}

void Config::setLedBrightness(uint8_t brightness) {
    if (conf.ledBrightness != brightness) {
        conf.ledBrightness = brightness;
        modified = true;
    }
}

void Config::setLedSpeed(uint8_t speed) {
    if (conf.ledSpeed != speed) {
        conf.ledSpeed = speed;
        modified = true;
    }
}

void Config::setLedColor(uint32_t color) {
    if (conf.ledColor != color) {
        conf.ledColor = color;
        modified = true;
    }
}

void Config::setLedFadeColor(uint32_t color) {
    if (conf.ledFadeColor != color) {
        conf.ledFadeColor = color;
        modified = true;
    }
}

void Config::setLedStrobeColor(uint32_t color) {
    if (conf.ledStrobeColor != color) {
        conf.ledStrobeColor = color;
        modified = true;
    }
}

void Config::setLedManualOverride(uint8_t override) {
    if (conf.ledManualOverride != override) {
        conf.ledManualOverride = override;
        modified = true;
    }
}

void Config::setTracksEnabled(uint8_t enabled) {
    if (conf.tracksEnabled != enabled) {
        conf.tracksEnabled = enabled;
        modified = true;
    }
}

void Config::setSelectedTrackId(uint32_t trackId) {
    if (conf.selectedTrackId != trackId) {
        conf.selectedTrackId = trackId;
        modified = true;
    }
}

void Config::setWebhooksEnabled(uint8_t enabled) {
    if (conf.webhooksEnabled != enabled) {
        conf.webhooksEnabled = enabled;
        modified = true;
    }
}

bool Config::addWebhookIP(const char* ip) {
    if (conf.webhookCount >= 10) {
        DEBUG("Max webhooks reached\n");
        return false;
    }
    
    // Check if IP already exists
    for (uint8_t i = 0; i < conf.webhookCount; i++) {
        if (strcmp(conf.webhookIPs[i], ip) == 0) {
            DEBUG("Webhook IP already exists\n");
            return false;
        }
    }
    
    strlcpy(conf.webhookIPs[conf.webhookCount], ip, 16);
    conf.webhookCount++;
    modified = true;
    return true;
}

bool Config::removeWebhookIP(const char* ip) {
    for (uint8_t i = 0; i < conf.webhookCount; i++) {
        if (strcmp(conf.webhookIPs[i], ip) == 0) {
            // Shift remaining IPs down
            for (uint8_t j = i; j < conf.webhookCount - 1; j++) {
                strlcpy(conf.webhookIPs[j], conf.webhookIPs[j + 1], 16);
            }
            conf.webhookCount--;
            memset(conf.webhookIPs[conf.webhookCount], 0, 16);  // Clear last entry
            modified = true;
            return true;
        }
    }
    return false;
}

void Config::clearWebhookIPs() {
    memset(conf.webhookIPs, 0, sizeof(conf.webhookIPs));
    conf.webhookCount = 0;
    modified = true;
}

void Config::setGateLEDsEnabled(uint8_t enabled) {
    if (conf.gateLEDsEnabled != enabled) {
        conf.gateLEDsEnabled = enabled;
        modified = true;
    }
}

void Config::setWebhookRaceStart(uint8_t enabled) {
    if (conf.webhookRaceStart != enabled) {
        conf.webhookRaceStart = enabled;
        modified = true;
    }
}

void Config::setWebhookRaceStop(uint8_t enabled) {
    if (conf.webhookRaceStop != enabled) {
        conf.webhookRaceStop = enabled;
        modified = true;
    }
}

void Config::setWebhookLap(uint8_t enabled) {
    if (conf.webhookLap != enabled) {
        conf.webhookLap = enabled;
        modified = true;
    }
}

void Config::setDefaults(void) {
    DEBUG("Setting EEPROM defaults\n");
    // Reset everything to 0/false and then just set anything that zero is not appropriate
    memset(&conf, 0, sizeof(conf));
    conf.version = CONFIG_VERSION | CONFIG_MAGIC;
    conf.frequency = 5658;  // RaceBand Channel 1 (R1) - 5658 MHz
    conf.minLap = 50;  // 5.0 seconds
    conf.alarm = 36;
    conf.announcerType = 2;
    conf.announcerRate = 10;
    conf.enterRssi = 120;
    conf.exitRssi = 100;
    conf.maxLaps = 0;
    conf.ledMode = 3;  // Rainbow wave by default (legacy)
    conf.ledBrightness = 80;
    conf.ledColor = 0xFF00FF;  // Purple by default
    conf.ledPreset = 2;  // Rainbow Wave preset by default
    conf.ledSpeed = 5;  // Medium speed
    conf.ledFadeColor = 0x0080FF;  // Blue for fade
    conf.ledStrobeColor = 0xFFFFFF;  // White for strobe
    conf.ledManualOverride = 0;  // Manual override off by default
    conf.operationMode = 0;  // WiFi mode by default
    conf.tracksEnabled = 0;  // Tracks disabled by default
    conf.selectedTrackId = 0;  // No track selected by default
    conf.webhooksEnabled = 0;  // Webhooks disabled by default
    conf.webhookCount = 0;  // No webhooks configured
    memset(conf.webhookIPs, 0, sizeof(conf.webhookIPs));  // Clear all webhook IPs
    conf.gateLEDsEnabled = 0;  // Gate LEDs disabled by default
    conf.webhookRaceStart = 1;  // Race start enabled by default
    conf.webhookRaceStop = 1;  // Race stop enabled by default
    conf.webhookLap = 1;  // Lap enabled by default
    strlcpy(conf.ssid, "", sizeof(conf.ssid));
    strlcpy(conf.password, "", sizeof(conf.password));
    strlcpy(conf.pilotName, "", sizeof(conf.pilotName));
    modified = true;
    write();
}

void Config::handleEeprom(uint32_t currentTimeMs) {
    if (modified && ((currentTimeMs - checkTimeMs) > EEPROM_CHECK_TIME_MS)) {
        checkTimeMs = currentTimeMs;
        write();
    }
}
