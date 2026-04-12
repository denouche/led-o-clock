#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_mac.h>
#include <WebServer.h>
#include <WiFiManager.h>

#include "api_handlers.h"
#include "config.h"
#include "led_control.h"
#include "ota_service.h"
#include "storage.h"

extern const uint8_t index_html_start[] asm("_binary_src_web_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_src_web_index_html_end");

extern const uint8_t index_js_start[] asm("_binary_src_web_index_js_start");
extern const uint8_t index_js_end[]   asm("_binary_src_web_index_js_end");

extern const uint8_t configuration_html_start[] asm("_binary_src_web_configuration_html_start");
extern const uint8_t configuration_html_end[]   asm("_binary_src_web_configuration_html_end");

extern const uint8_t configuration_js_start[] asm("_binary_src_web_configuration_js_start");
extern const uint8_t configuration_js_end[]   asm("_binary_src_web_configuration_js_end");

extern const uint8_t common_css_start[]  asm("_binary_src_web_common_css_start");
extern const uint8_t common_css_end[]    asm("_binary_src_web_common_css_end");

extern const uint8_t common_js_start[]  asm("_binary_src_web_common_js_start");
extern const uint8_t common_js_end[]    asm("_binary_src_web_common_js_end");

static WebServer server(80);

/**
 * Helper to get a truly unique Chip ID based on full MAC
 */
String getUniqueChipId() {
    uint8_t mac[6];
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) != ESP_OK) {
        return "000000000000";
    }
    char chipIdBuf[13];
    snprintf(chipIdBuf, sizeof(chipIdBuf), "%02x%02x%02x%02x%02x%02x", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
             
    return String(chipIdBuf);
}

/**
 * Trims trailing null or 0xFF bytes from embedded binary data
 */
size_t getCleanSize(const uint8_t* start, const uint8_t* end) {
    const char* data = (const char*)start;
    size_t size = end - start;
    while (size > 0 && (data[size - 1] == 0x00 || (uint8_t)data[size - 1] == 0xFF)) {
        size--;
    }
    return size;
}

/**
 * Serves a static file directly from Flash with caching headers
 */
void serveStaticEmbed(const uint8_t* start, const uint8_t* end, const char* contentType) {
    size_t size = getCleanSize(start, end);
    String etag = "\"" + String(FIRMWARE_VERSION) + "\"";

    if (server.hasHeader("If-None-Match") && server.header("If-None-Match") == etag) {
        server.sendHeader("Cache-Control", "public, max-age=86400");
        server.sendHeader("ETag", etag);
        server.send(304);
        return;
    }

    server.sendHeader("Cache-Control", "public, max-age=86400");
    server.sendHeader("ETag", etag);
    server.send_P(200, contentType, (const char*)start, size);
}

/**
 * Returns a clean String from embedded Flash data (for placeholders replacement)
 */
String getEmbedString(const uint8_t* start, const uint8_t* end) {
    size_t size = getCleanSize(start, end);
    String s = "";
    s.concat((const char*)start, size);
    return s;
}

void handleRoot() {
    Serial.println("handleRoot");

    String chipId = getUniqueChipId();
    String hostname = "ledoclock-" + chipId;
    String mac = WiFi.macAddress();
    String ip = WiFi.localIP().toString();

    String etag = "\"" + String(FIRMWARE_VERSION) + "-" + ip + "\"";
    if (server.hasHeader("If-None-Match") && server.header("If-None-Match") == etag) {
        server.sendHeader("Cache-Control", "no-cache");
        server.sendHeader("ETag", etag);
        server.send(304);
        return;
    }

    // load index.html from Flash and replace placeholders
    String html = getEmbedString(index_html_start, index_html_end);
    if (html.length() == 0) {
        Serial.println("Error: Could not load index.html from Flash");
        server.send(500, "text/plain", "Internal Server Error");
        return;
    }

    html.replace("%HOSTNAME%", hostname);
    html.replace("%MAC%", mac);
    html.replace("%IP%", ip);
    html.replace("%VERSION%", String(FIRMWARE_VERSION));

    server.sendHeader("Cache-Control", "no-cache");
    server.sendHeader("ETag", etag);
    server.send(200, "text/html", html);
}


void handlePing() {
    Serial.println("handlePing");
    server.send(200, "application/json", "{\"status\":\"pong\"}");
}

void handleConfigure() {
    Serial.println("handleConfigure");
    serveStaticEmbed(configuration_html_start, configuration_html_end, "text/html");
}

/**
 * Clears WiFi credentials and restarts the device
 * to force the configuration portal on next boot.
 */
void handleResetWifi() {
    Serial.println("handleResetWifi");
    server.send(200, "application/json", "{\"status\":\"WiFi settings cleared. Unplug and plug the device to restart in Access Point mode.\"}");
    // Give the server time to send the response
    delay(2000);

    WiFiManager wm;
    wm.resetSettings();
    ESP.restart();
}

/**
 * Clears all configurations, WiFi credentials and restarts the device
 * to force the configuration portal on next boot.
 */
void handleResetAll() {
    Serial.println("handleResetAll");
    server.send(200, "application/json", "{\"status\":\"Settings cleared. Unplug and plug the device to restart in Access Point mode.\"}");
    // Give the server time to send the response
    delay(1000);

    showFactoryResetFeedback();
    performFactoryReset();
}

void handleGetStatus() {
    Serial.println("handleGetStatus");
    
    JsonDocument doc;
    doc["version"] = FIRMWARE_VERSION;
    doc["brightness_raw"] = globalBrightness;
    doc["brightness_percent"] = (globalBrightness * 100 + 127) / 255; // Convert 0-255 to 0-100% with rounding
    doc["current_color"] = currentColorMode;
    doc["timezone"] = currentTimezone;
    
    // Send hardware state
    doc["num_leds"] = NUM_LEDS;
    doc["current_leds_on"] = currentLedsOn;

    // Send current ESP time
    time_t now = time(nullptr);
    struct tm* p_tm = localtime(&now);
    
    if (p_tm->tm_year >= 100) {
        char timeStr[9];
        sprintf(timeStr, "%02d:%02d:%02d", p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
        doc["esp_time"] = timeStr;
    } else {
        doc["esp_time"] = ""; // Not synced yet
    }

    // Add Colors to JSON
    JsonArray colorArray = doc["colors"].to<JsonArray>();
    for (int i = 0; i < definedColorCount; i++) {
        JsonObject c = colorArray.add<JsonObject>();
        c["name"] = definedColors[i].name;
        
        char hex[8];
        sprintf(hex, "#%02X%02X%02X", definedColors[i].r, definedColors[i].g, definedColors[i].b);
        c["hex"] = String(hex);
    }
    
    // Add Schedules to JSON
    JsonArray schedArray = doc["schedules"].to<JsonArray>();
    for (int i = 0; i < scheduleCount; i++) {
        JsonObject s = schedArray.add<JsonObject>();
        char timeBuf[6];
        sprintf(timeBuf, "%02d:%02d", schedules[i].hour, schedules[i].minute);
        s["time"] = timeBuf;
        s["color"] = schedules[i].color;
        s["countdown"] = schedules[i].countdown;
        s["brightness"] = schedules[i].brightness;
        
        JsonArray daysArr = s["days"].to<JsonArray>();
        for (int d = 0; d < 7; d++) {
            daysArr.add(schedules[i].days[d]);
        }
    }

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleSetColor() {
    Serial.println("handleSetColor");
    if (server.hasArg("value")) {
        applyColor(server.arg("value"));
        server.send(200, "application/json", "{\"status\":\"saved\"}");
    } else {
        server.send(400, "application/json", "{\"error\":\"Missing value parameter\"}");
    }
}

void handleSetBrightness() {
    Serial.println("handleSetBrightness");
    if (server.hasArg("value")) {
        // Get percentage from 0 to 100
        int percent = server.arg("value").toInt();
        percent = constrain(percent, 0, 100);
        
        // Map 0-100% to 0-255 range
        globalBrightness = (percent * 255) / 100;
        
        saveBrightness(globalBrightness);
        
        // Refresh the current light with the new intensity
        applyColor(currentColorMode);
        server.send(200, "application/json", "{\"status\":\"saved\"}");
    } else {
        server.send(400, "application/json", "{\"error\":\"Missing value parameter\"}");
    }
}

void handlePostSchedule() {
    Serial.println("handlePostSchedule");
    
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"Body Missing\"}");
        
        return;
    }

    String payload = server.arg("plain");
    
    saveSchedulesToFS(payload);
    loadSchedulesFromFS();

    // Force immediate recalculation of the state based on the new schedules
    forceStateEvaluation();
    
    // Immediately process the countdown calculation to avoid race conditions 
    // when the frontend fetches /status right after saving
    updateLedRing();
    
    server.send(200, "application/json", "{\"status\":\"saved\"}");
}

void handleSetTimezone() {
    Serial.println("handleSetTimezone");
    if (!server.hasArg("value")) {
        server.send(400, "application/json", "{\"error\":\"Missing value parameter\"}");
        return;
    }

    String tz = server.arg("value");
    currentTimezone = tz;
    saveTimezoneToFS(tz);
    
    // Apply the new timezone
    Serial.println("Setting timezone to: " + tz);
    configTzTime(currentTimezone.c_str(), "pool.ntp.org");
    
    server.send(200, "application/json", "{\"status\":\"saved\"}");
}

void handlePostColors() {
    Serial.println("handlePostColors");
    
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"Body Missing\"}");
        return;
    }

    String payload = server.arg("plain");
    
    saveColorsToFS(payload);
    loadColorsFromFS();
    
    // Update live ring instantly in case current color definition was modified
    applyColor(currentColorMode);
    
    server.send(200, "application/json", "{\"status\":\"saved\"}");
}

void handlePostFirmwareUpdate() {
    Serial.println("handlePostFirmwareUpdate");
    server.send(204);
    // Give the server time to send the response
    delay(2000);
    updateFirmwareIfNeeded();
}

void handleFavicon() {
    server.send(204);
}

void initEndpoints() {
    server.on("/",                HTTP_GET,  handleRoot);
    server.on("/colors",          HTTP_POST, handlePostColors);
    server.on("/configure",       HTTP_GET,  handleConfigure);
    server.on("/favicon.ico",     HTTP_GET,  handleFavicon);
    server.on("/ping",            HTTP_GET,  handlePing);
    server.on("/reset_wifi",      HTTP_GET,  handleResetWifi);
    server.on("/reset",           HTTP_GET,  handleResetAll);
    server.on("/schedule",        HTTP_POST, handlePostSchedule);
    server.on("/set_brightness",  HTTP_GET,  handleSetBrightness);
    server.on("/set_color",       HTTP_GET,  handleSetColor);
    server.on("/set_timezone",    HTTP_GET,  handleSetTimezone);
    server.on("/status",          HTTP_GET,  handleGetStatus);
    server.on("/firmware_update", HTTP_POST, handlePostFirmwareUpdate);

    // --- Static Web Assets (Streamed directly from PROGMEM) ---
    server.on("/common.css", HTTP_GET, []() {
        serveStaticEmbed(common_css_start, common_css_end, "text/css");
    });

    server.on("/common.js", HTTP_GET, []() {
        serveStaticEmbed(common_js_start, common_js_end, "text/javascript");
    });

    server.on("/index.js", HTTP_GET, []() {
        serveStaticEmbed(index_js_start, index_js_end, "text/javascript");
    });

    server.on("/configuration.js", HTTP_GET, []() {
        serveStaticEmbed(configuration_js_start, configuration_js_end, "text/javascript");
    });

    const char* headerKeys[] = { "If-None-Match" };
    server.collectHeaders(headerKeys, 1);
    server.begin();
}

void handleWebClient() {
    server.handleClient();
}
