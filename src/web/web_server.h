#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <FS.h>  // Include FS.h first to ensure proper namespace resolution
using fs::FS;    // Bring FS into global namespace for WebServer.h compatibility
#include <WebServer.h>
#include <SPIFFS.h>
#include <vector>
#include "timing_core.h"

// Forward declaration
class SettingsManager;

// Web server manager for standalone mode
// Handles HTTP server setup, routing, and API endpoints
class WebServerManager {
public:
    WebServerManager();

    // Initialize web server with routes and start listening
    // Must be called after WiFi AP is up
    void begin(TimingCore* timingCore, SettingsManager* settingsManager,
               bool* raceActive, uint32_t* raceStartTime, std::vector<LapData>* laps);

    // Handle incoming client requests (call from task loop)
    void handleClient();

    // Start dedicated web server task
    void startTask();

private:
    WebServer _server;
    TimingCore* _timingCore;
    SettingsManager* _settingsManager;

    // Race state (shared with StandaloneMode)
    bool* _raceActive;
    uint32_t* _raceStartTime;
    std::vector<LapData>* _laps;

    TaskHandle_t _webTaskHandle;

    // Static task wrapper
    static void webServerTask(void* parameter);

    // HTTP handlers
    void handleRoot();
    void handleGetStatus();
    void handleGetLaps();
    void handleStartRace();
    void handleStopRace();
    void handleClearLaps();
    void handleSetFrequency();
    void handleSetThreshold();
    void handleGetChannels();
    void handleGetSPIFFSInfo();
    void handleGetConfig();
    void handleStyleCSS();
    void handleAppJS();
    void handleNotFound();
};

#endif // WEB_SERVER_H
