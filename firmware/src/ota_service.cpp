#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>

#include "config.h"
#include "led_control.h"
#include "ota_service.h"


FirmwareInfo getLatestFirmwareInfo() {
    FirmwareInfo info;
    char url[80];
    snprintf(url, sizeof(url), "https://api.github.com/repos/denouche/led-o-clock/releases/latest");
    
    HTTPClient http;

    WiFiClientSecure secureClient;
    secureClient.setInsecure();
    bool success = http.begin(secureClient, url);

    if (!success) {
        Serial.println("getLatestFirmwareVersion error while begin http");
        return info;
    }

    http.addHeader("User-Agent", "Led-O-Clock-V3");
    
    int code = http.GET();

    if (code == 200) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, http.getStream());
        if (!error && doc["tag_name"].is<String>()) {
            info.latestVersion = doc["tag_name"].as<String>();
            Serial.printf("Latest firmware version available: %s\n", info.latestVersion.c_str());
        }

        // Only proceed if versions don't match
        Serial.printf("Current firmware version: %s\n", FIRMWARE_VERSION);
        if (info.latestVersion != "" && info.latestVersion != FIRMWARE_VERSION) {
            Serial.println("Should update firmware...");

            JsonArray assets = doc["assets"].as<JsonArray>();
            for (JsonObject asset : assets) {
                if (asset["name"].as<String>() == "firmware.bin") {
                    info.downloadUrl = asset["browser_download_url"].as<String>();
                    break;
                }
            }
        }
    } else {
        String response = http.getString();
        Serial.printf("Firmware check failed with code %d, body: %s\n", code, response.c_str());
    }

    http.end();
    return info;
}

/**
 * Perform the actual firmware update from a given URL
 */
void updateFirmware(String downloadUrl) {
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
