#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

// WiFi Access Point manager for standalone mode
// Handles WiFi AP setup with unique SSID based on MAC address
class WiFiManager {
public:
    WiFiManager();

    // Setup WiFi Access Point
    // Returns true if AP started successfully
    bool setupAP();

    // Get the current AP SSID
    String getSSID() const { return _apSSID; }

private:
    String _apSSID;
};

#endif // WIFI_MANAGER_H
