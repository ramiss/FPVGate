#include "settings_manager.h"
#include "config/config.h"
#include <Preferences.h>

SettingsManager::SettingsManager() {
    // Constructor - no initialization needed
}

bool SettingsManager::loadSettings(TimingCore* timingCore) {
    if (!timingCore) return false;

    Preferences prefs;

    // Open preferences in read-only mode
    if (!prefs.begin("sfos", true)) {
        Serial.println("Failed to open preferences for reading (NVS may not be initialized)");
        Serial.println("This is normal on first boot - settings will be saved after first change");
        return false;
    }

    // Check if settings exist (using a magic number to verify initialization)
    uint32_t magic = prefs.getUInt("magic", 0);
    if (magic != MAGIC_NUMBER) {
        Serial.println("No saved settings found (magic number mismatch or first boot), using defaults");
        Serial.printf("Expected magic: 0x%08X, Found: 0x%08X\n", MAGIC_NUMBER, magic);
        prefs.end();
        return false;
    }

    // Load settings
    uint8_t band = prefs.getUChar("band", 0);
    uint8_t channel = prefs.getUChar("channel", 0);

    // Try to load dual thresholds (new format)
    uint8_t enter_rssi = prefs.getUChar("enter_rssi", 0);
    uint8_t exit_rssi = prefs.getUChar("exit_rssi", 0);

    // If dual thresholds not found, try legacy single threshold
    if (enter_rssi == 0 && exit_rssi == 0) {
        uint8_t threshold = prefs.getUChar("threshold", ENTER_RSSI);
        // Migrate: enter = threshold, exit = threshold - 20 (or threshold if too low)
        enter_rssi = threshold;
        exit_rssi = (threshold > 20) ? (threshold - 20) : threshold;
    }

    // If still zero, use defaults
    if (enter_rssi == 0) enter_rssi = ENTER_RSSI;
    if (exit_rssi == 0) exit_rssi = EXIT_RSSI;

    prefs.end();

    // Apply loaded settings to timing core
    timingCore->setRX5808Settings(band, channel);
    timingCore->setEnterRssi(enter_rssi);
    timingCore->setExitRssi(exit_rssi);

    Serial.println("\n=== Loaded Settings from Flash ===");
    Serial.printf("Band: %d, Channel: %d\n", band, channel + 1);
    Serial.printf("Frequency: %d MHz\n", timingCore->getCurrentFrequency());
    Serial.printf("Enter RSSI: %d, Exit RSSI: %d\n", enter_rssi, exit_rssi);
    Serial.println("===================================\n");

    return true;
}

bool SettingsManager::saveSettings(TimingCore* timingCore) {
    if (!timingCore) return false;

    Preferences prefs;

    // Try to open preferences in read-write mode with retry logic
    // Sometimes NVS needs a small delay if it was just accessed
    bool opened = false;
    for (int attempt = 0; attempt < 3; attempt++) {
        if (attempt > 0) {
            delay(10);  // Small delay between retries
        }
        opened = prefs.begin("sfos", false);
        if (opened) break;
    }

    if (!opened) {
        Serial.println("ERROR: Failed to open preferences for writing after 3 attempts");
        Serial.println("This may indicate NVS partition issue or namespace conflict");
        Serial.printf("NVS namespace: sfos, mode: read-write\n");
        return false;
    }

    // Get current settings from timing core
    uint8_t band, channel;
    timingCore->getRX5808Settings(band, channel);
    uint8_t enter_rssi = timingCore->getEnterRssi();
    uint8_t exit_rssi = timingCore->getExitRssi();

    // Save magic number to indicate settings are valid
    bool magicOk = prefs.putUInt("magic", MAGIC_NUMBER);
    if (!magicOk) {
        Serial.println("ERROR: Failed to write magic number");
        prefs.end();
        return false;
    }

    // Save settings
    prefs.putUChar("band", band);
    prefs.putUChar("channel", channel);
    prefs.putUChar("enter_rssi", enter_rssi);
    prefs.putUChar("exit_rssi", exit_rssi);
    // Keep legacy threshold for backward compatibility
    prefs.putUChar("threshold", enter_rssi);

    prefs.end();

    Serial.printf("Settings saved to flash: Band=%d, Channel=%d, Enter RSSI=%d, Exit RSSI=%d\n",
                  band, channel, enter_rssi, exit_rssi);

    return true;
}
