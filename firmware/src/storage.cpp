#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <WiFiManager.h>

#include "config.h"
#include "storage.h"

const char* SCHEDULE_FILE = "/schedules.json";
const char* TIMEZONE_FILE = "/timezone.txt";
const char* COLORS_FILE = "/colors.json";

Preferences prefs;

// --- Helper Functions ---

/**
 * Sorts the schedules array chronologically (Bubble sort is enough for < 10 items)
 */
void sortSchedules() {
    for (int i = 0; i < scheduleCount - 1; i++) {
        for (int j = 0; j < scheduleCount - i - 1; j++) {
            int timeA = (schedules[j].hour * 60) + schedules[j].minute;
            int timeB = (schedules[j + 1].hour * 60) + schedules[j + 1].minute;
            
            if (timeA > timeB) {
                // Swap
                Schedule temp = schedules[j];
                schedules[j] = schedules[j + 1];
                schedules[j + 1] = temp;
            }
        }
    }
}

void initPreferences() {
    // Open the "clock" namespace in Read/Write mode
    prefs.begin("clock", false);
}

void saveBrightness(int brightness) {
    prefs.putInt("brightness", brightness);
}

int loadBrightness() {
    // Return 127 as default if the key does not exist yet
    return prefs.getInt("brightness", 127);
}

void saveColorModeIndex(int index) {
    prefs.putInt("colorIndex", index);
}

int loadColorModeIndex() {
    // Return 255 as default if no color was saved
    return prefs.getInt("colorIndex", 255);
}

void clearPreferences() {
    prefs.clear();
}

// --- Storage Functions (LittleFS) ---

/**
 * Load colors from LittleFS or set defaults
 */
void loadColorsFromFS() {
    if (!LittleFS.exists(COLORS_FILE)) {
        // Set default Red
        definedColors[0].name = "red";
        definedColors[0].r = 255;
        definedColors[0].g = 0;
        definedColors[0].b = 0;

        // Set default Orange
        definedColors[1].name = "orange";
        definedColors[1].r = 255;
        definedColors[1].g = 64; 
        definedColors[1].b = 0;

        // Set default Green
        definedColors[2].name = "green";
        definedColors[2].r = 0;
        definedColors[2].g = 255;
        definedColors[2].b = 0;

        definedColorCount = 3;
        
        return;
    }

    File file = LittleFS.open(COLORS_FILE, "r");
    
    if (!file) {
        return;
    }

    String content = file.readString();
    file.close();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, content);

    if (!error) {
        JsonArray array = doc.as<JsonArray>();
        definedColorCount = 0;
        
        for (JsonObject obj : array) {
            if (definedColorCount < MAX_COLORS) {
                definedColors[definedColorCount].name = obj["name"].as<String>();
                
                String hex = obj["hex"].as<String>();
                
                if (hex.startsWith("#")) {
                    hex.remove(0, 1);
                }
                
                long number = strtol(hex.c_str(), nullptr, 16);
                definedColors[definedColorCount].r = (number >> 16) & 0xFF;
                definedColors[definedColorCount].g = (number >> 8) & 0xFF;
                definedColors[definedColorCount].b = number & 0xFF;
                
                definedColorCount++;
            }
        }
    }
}

/**
 * Save custom colors to LittleFS
 */
void saveColorsToFS(String json) {
    File file = LittleFS.open(COLORS_FILE, "w");
    
    if (file) {
        file.print(json);
        file.close();
    }
}

void saveSchedulesToFS(String json) {
    File file = LittleFS.open(SCHEDULE_FILE, "w");
    if (file) {
        file.print(json);
        file.close();
    }
}

void loadSchedulesFromFS() {
    if (!LittleFS.exists(SCHEDULE_FILE)) return;

    File file = LittleFS.open(SCHEDULE_FILE, "r");
    if (!file) return;

    String content = file.readString();
    
    file.close();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, content);

    if (!error) {
        JsonArray array = doc.as<JsonArray>();
        scheduleCount = 0;
        
        for (JsonObject obj : array) {
            if (scheduleCount < MAX_SCHEDULES) {
                String timeStr = obj["time"]; 

                schedules[scheduleCount].hour   = timeStr.substring(0, 2).toInt();
                schedules[scheduleCount].minute = timeStr.substring(3, 5).toInt();
                schedules[scheduleCount].color  = obj["color"].as<String>();
                schedules[scheduleCount].active = true;
                schedules[scheduleCount].countdown = obj["countdown"] | false;
                schedules[scheduleCount].brightness = obj["brightness"] | 50;
                
                // Read the 7 days array if it exists
                if (obj["days"].is<JsonArray>()) {
                    JsonArray daysArr = obj["days"];
                    
                    for (int d = 0; d < 7; d++) {
                        schedules[scheduleCount].days[d] = daysArr[d].as<bool>();
                    }
                } else {
                    // Fallback for older saves: apply to all days
                    for (int d = 0; d < 7; d++) {
                        schedules[scheduleCount].days[d] = true;
                    }
                }

                scheduleCount++;
            }
        }

        // Sort schedules chronologically
        sortSchedules();
    }
}

void saveTimezoneToFS(String tz) {
    File file = LittleFS.open(TIMEZONE_FILE, "w");
    if (file) {
        file.print(tz);
        file.close();
    }
}

void loadTimezoneFromFS() {
    if (!LittleFS.exists(TIMEZONE_FILE)) return;

    File file = LittleFS.open(TIMEZONE_FILE, "r");
    if (!file) return;

    String content = file.readString();
    file.close();

    content.trim(); // Remove whitespace/newlines
    if (content.length() > 0) {
        currentTimezone = content;
    }
}

void cleanupStorage() {
    if (LittleFS.exists(SCHEDULE_FILE)) {
        LittleFS.remove(SCHEDULE_FILE);
    }
    
    if (LittleFS.exists(TIMEZONE_FILE)) {
        LittleFS.remove(TIMEZONE_FILE);
    }
    
    if (LittleFS.exists(COLORS_FILE)) {
        LittleFS.remove(COLORS_FILE);
    }
}

/**
 * Performs a complete factory reset: WiFi, Filesystem, and EEPROM
 */
void performFactoryReset() {
    Serial.println("Performing Factory Reset...");

    // 1. Clear WiFi settings
    WiFiManager wm;
    wm.resetSettings();

    // 2. Clear LittleFS saved files
    cleanupStorage();

    // 3. Clear Preferences (NVS)
    clearPreferences();

    Serial.println("Factory Reset complete. Restarting...");
    
    // Give time for Serial to flush
    delay(1000); 
    ESP.restart();
}
