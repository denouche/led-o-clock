#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>

#include "config.h"
#include "led_control.h"
#include "ota_service.h"

const char* MANIFEST_URL = "https://denouche.github.io/led-o-clock/latest_firmware.json";

FirmwareInfo getLatestFirmwareInfo() {
    FirmwareInfo info;
    info.latestVersion = "";
    info.downloadUrl = "";

    WiFiClientSecure secureClient;
    secureClient.setInsecure();

    HTTPClient http;
    http.setTimeout(10000);

    Serial.printf("Checking manifest at: %s\n", MANIFEST_URL);
    
    bool success = http.begin(secureClient, MANIFEST_URL);
    if (!success) {
        Serial.println("OTA: Unable to begin HTTP connection");
        return info;
    }

    http.addHeader("User-Agent", "led-o-clock/" + String(FIRMWARE_VERSION));
    
    int code = http.GET();
    if (code == 200) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, http.getStream());
        
        if (!error) {
            String remoteVersion = doc["version"].as<String>();
            String downloadUrl = doc["url"].as<String>();

            info.latestVersion = remoteVersion;
            
            Serial.printf("Current firmware version: %s\n", FIRMWARE_VERSION);
            Serial.printf("Latest firmware available: %s\n", remoteVersion.c_str());

            if (remoteVersion != "" && remoteVersion != String(FIRMWARE_VERSION)) {
                Serial.println("Update available!");
                info.downloadUrl = downloadUrl;
            }
        } else {
            Serial.printf("OTA: JSON parsing failed: %s\n", error.c_str());
        }
    } else {
        Serial.printf("OTA: Manifest check failed (HTTP %d)\n", code);
    }

    http.end();
    return info;
}

/**
 * Perform the actual firmware update from a given URL
 */
void updateFirmware(String downloadUrl) {
    if (downloadUrl == "") return;
    
    WiFiClientSecure client;
    client.setInsecure();

    // MANDATORY: Tell httpUpdate to follow GitHub's 302 redirection to Amazon S3
    httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    // Starting the update process
    Serial.println("Starting OTA transfer...");
    t_httpUpdate_return ret = httpUpdate.update(client, downloadUrl);

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", 
                          httpUpdate.getLastError(), 
                          httpUpdate.getLastErrorString().c_str());
            break;

        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            break;

        case HTTP_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK"); // May not be printed as ESP32 reboots
            break;
    }
}

void updateFirmwareIfNeeded() {
    FirmwareInfo info = getLatestFirmwareInfo();
    Serial.printf("Latest firmware URL: %s\n", info.downloadUrl.c_str());
    if (info.downloadUrl != "") {
        showFirmwareUpdateFeedback();
        delay(2000); // Allow user to see the purple light before starting the update
        updateFirmware(info.downloadUrl);
    } else {
        Serial.println("Got empty firmware URL. Skipping update.");
    }
}
