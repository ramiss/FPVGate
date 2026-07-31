#pragma once

// ── Reliable MAC address accessors ──────────────────────────────────────────
//
// Always read the MAC from eFuse via esp_read_mac() rather than calling
// WiFi.macAddress() / WiFi.softAPmacAddress().
//
// Why: the Arduino-ESP32 wrappers query the *running* WiFi driver interface,
// so their result depends on which netifs happen to be started:
//
//   * In AP-only mode (nodeMode 0 single and nodeMode 1 master both boot
//     WIFI_AP) the STA netif is never started, and WiFi.macAddress() returns
//     00:00:00:00:00:00.  This produced an all-zeros MAC in the Diagnostics
//     page, in the master's node-list entry (which keys the pilot backup /
//     restore format), and in the mDNS instance name (making the "unique"
//     instance identical on every unit).
//   * In WIFI_MODE_NULL (before any WiFi.mode() call) the same wrapper has
//     returned an IDENTICAL string on distinct ESP32-C6 chips — the bug that
//     made two multi-node clients collide on slot 1.  See multinode.cpp.
//
// esp_read_mac() reads the factory-programmed eFuse block directly.  It is
// valid before WiFi starts, in every WiFi mode, and is guaranteed unique per
// chip.  webserver.cpp already relied on this for the SoftAP SSID.
//
// ESP_MAC_WIFI_SOFTAP is derived from the STA base MAC (typically +1 in the
// last octet), so the two accessors below return different values — use the
// one matching the interface you are describing.

#include <Arduino.h>
#include <esp_mac.h>

// Format a 6-byte MAC as uppercase colon-separated text (AA:BB:CC:DD:EE:FF).
inline String macToString(const uint8_t mac[6]) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

// This chip's station-interface MAC.  Use as the device's stable identity —
// node registration, backup/restore keys, mDNS instance names.
inline String getStaMacString() {
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    return macToString(mac);
}

// This chip's SoftAP-interface MAC.  Use only when specifically describing
// the AP interface (e.g. what a client sees as the BSSID).
inline String getApMacString() {
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    return macToString(mac);
}
