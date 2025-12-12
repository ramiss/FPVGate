#include "webhook.h"
#include "debug.h"

WebhookManager::WebhookManager() {
    enabled = true;
}

bool WebhookManager::addWebhook(const String& ip) {
    // Check if already exists
    for (const auto& existingIp : webhookIPs) {
        if (existingIp == ip) {
            DEBUG("Webhook IP already exists: %s\n", ip.c_str());
            return false;
        }
    }
    
    // Check max limit
    if (webhookIPs.size() >= MAX_WEBHOOKS) {
        DEBUG("Max webhooks reached (%d)\n", MAX_WEBHOOKS);
        return false;
    }
    
    webhookIPs.push_back(ip);
    DEBUG("Webhook added: %s\n", ip.c_str());
    return true;
}

bool WebhookManager::removeWebhook(const String& ip) {
    for (auto it = webhookIPs.begin(); it != webhookIPs.end(); ++it) {
        if (*it == ip) {
            webhookIPs.erase(it);
            DEBUG("Webhook removed: %s\n", ip.c_str());
            return true;
        }
    }
    DEBUG("Webhook not found: %s\n", ip.c_str());
    return false;
}

void WebhookManager::clearWebhooks() {
    webhookIPs.clear();
    DEBUG("All webhooks cleared\n");
}

std::vector<String> WebhookManager::getWebhooks() const {
    return webhookIPs;
}

void WebhookManager::setEnabled(bool en) {
    enabled = en;
    DEBUG("Webhooks %s\n", enabled ? "enabled" : "disabled");
}

bool WebhookManager::isEnabled() const {
    return enabled;
}

void WebhookManager::triggerLap() {
    if (!enabled) return;
    DEBUG("Triggering webhook: /Lap\n");
    sendToAll("/Lap");
}

void WebhookManager::triggerGhostLap() {
    if (!enabled) return;
    DEBUG("Triggering webhook: /GhostLap\n");
    sendToAll("/GhostLap");
}

void WebhookManager::triggerRaceStart() {
    if (!enabled) return;
    DEBUG("Triggering webhook: /RaceStart\n");
    sendToAll("/RaceStart");
}

void WebhookManager::triggerRaceStop() {
    if (!enabled) return;
    DEBUG("Triggering webhook: /RaceStop\n");
    sendToAll("/RaceStop");
}

void WebhookManager::triggerOff() {
    if (!enabled) return;
    DEBUG("Triggering webhook: /off\n");
    sendToAll("/off");
}

void WebhookManager::triggerFlash() {
    if (!enabled) return;
    DEBUG("Triggering webhook: /flash\n");
    sendToAll("/flash");
}

void WebhookManager::sendToAll(const String& endpoint) {
    for (const auto& ip : webhookIPs) {
        sendWebhook(ip, endpoint);
    }
}

void WebhookManager::sendWebhook(const String& ip, const String& endpoint) {
    HTTPClient http;
    String url = "http://" + ip + endpoint;
    
    http.setTimeout(WEBHOOK_TIMEOUT_MS);
    http.begin(url);
    
    int httpCode = http.POST("");  // Empty POST body
    
    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_ACCEPTED) {
            DEBUG("Webhook success: %s (code: %d)\n", url.c_str(), httpCode);
        } else {
            DEBUG("Webhook returned code %d: %s\n", httpCode, url.c_str());
        }
    } else {
        DEBUG("Webhook failed: %s (error: %s)\n", url.c_str(), http.errorToString(httpCode).c_str());
    }
    
    http.end();
}
