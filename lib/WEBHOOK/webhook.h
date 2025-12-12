#ifndef WEBHOOK_H
#define WEBHOOK_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <vector>

#define MAX_WEBHOOKS 10
#define WEBHOOK_TIMEOUT_MS 500  // Short timeout to avoid blocking

class WebhookManager {
   public:
    WebhookManager();
    
    // Add/remove webhook IPs
    bool addWebhook(const String& ip);
    bool removeWebhook(const String& ip);
    void clearWebhooks();
    std::vector<String> getWebhooks() const;
    
    // Trigger webhook events (non-blocking POST requests)
    void triggerLap();
    void triggerGhostLap();
    void triggerRaceStart();
    void triggerRaceStop();
    void triggerOff();
    void triggerFlash();
    
    // Enable/disable webhooks
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
   private:
    std::vector<String> webhookIPs;
    bool enabled;
    
    // Internal method to send POST request to all webhooks
    void sendToAll(const String& endpoint);
    
    // Send to specific IP (non-blocking)
    void sendWebhook(const String& ip, const String& endpoint);
};

#endif // WEBHOOK_H
