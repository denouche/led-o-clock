/**
 * Project: Led'o'clock
 * Hardware: ESP32-C3-WROOM-02
 * Display: WS2812B LED Ring on GPIO2
 * Author: Antoine Leveugle
 * 
 * * Note: GPIO9 button is used for both Flash Mode (at boot) and WiFi Reset (long press during runtime).
 * 
 * * LED Status Signals:
 * - Light Blue: System booting and attempting to connect to WiFi.
 * - Yellow: WiFi Configuration mode (Access Point active).
 * - Green (Blinks 5 times): WiFi connection successful.
 * - Purple: Firmware update in progress.
 * - Red (Blinks 5 times): Factory Reset in progress.
 */

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <LittleFS.h>
#include <time.h>
#include <WiFiManager.h>

#include "config.h"
#include "storage.h"
#include "led_control.h"
#include "api_handlers.h"
#include "web/wifi_manager_html.h"

#ifndef BUILD_TAG
    #define BUILD_TAG "build-dev"
#endif
const char* FIRMWARE_VERSION = (BUILD_TAG[0] == '\0') ? "dev-build" : BUILD_TAG;

// --- State Management ---
Schedule schedules[MAX_SCHEDULES];
int scheduleCount = 0;

ColorDef definedColors[MAX_COLORS];
int definedColorCount = 0;

int globalBrightness = 128;
String currentColorMode = "off";
String currentTimezone = "CET-1CEST,M3.5.0,M10.5.0/3"; // Default timezone
int currentLedsOn = NUM_LEDS; // Track how many LEDs are currently lit to avoid redundant updates

/**
 * Configure WiFi Roaming for Mesh stability (ESP32 Specific)
 */
void configureWiFiRoaming() {
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);

    // Connect to the strongest AP in the mesh
    conf.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    conf.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
    
    // Threshold to trigger search for a better AP (in dBm)
    conf.sta.threshold.rssi = -75; 

    esp_wifi_set_config(WIFI_IF_STA, &conf);
}

/**
 * Check the physical button for a long press (>5s) and trigger a factory reset.
 * Extracted so it can be polled both during normal runtime (loop) and while the
 * device is in WiFi Access Point / config portal mode.
 */
void checkFactoryResetButton() {
    static unsigned long buttonPressStartTime = 0;

    if (digitalRead(PIN_BUTTON) == LOW) {
        if (buttonPressStartTime == 0) {
            buttonPressStartTime = millis();
        } else if (millis() - buttonPressStartTime > 5000) {
            showFactoryResetFeedback();
            performFactoryReset();
        }
    } else {
        buttonPressStartTime = 0;
    }
}

void setup() {
    Serial.begin(115200);
    delay(500); // Allow time for Native USB Serial to initialize

    Serial.println();
    Serial.println();
    Serial.println("================================");
    Serial.println("================================");
    Serial.println("STARTING Led'o'clock");
    Serial.println("================================");
    Serial.println("================================");

    // Hardware Setup
    initLedRing();
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    
    setBrightness(1);
    setRingRgb(0, 255, 255); // light blue to indicate device is booting and attempting WiFi connection

    // --- Filesystem Initialization ---
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS fatal error: Mount and format failed.");
    }

    Serial.println("Connecting to WiFi...");
    
    // Networking
    // For mDNS and network identity
    String chipId = getUniqueChipId();
    String hostname = "ledoclock-" + chipId;

    WiFi.setHostname(hostname.c_str());
    WiFiManager wm;

    wm.setAPCallback([](WiFiManager* w){
        Serial.println("Entered AP mode.");
        setBrightness(1);
        setRingRgb(255, 245, 0); // Yellow to indicate AP mode
    });

    wm.setSaveConfigCallback([](){
        showWifiSuccessFeedback();
        // we wait for 30sec after saving but before turning off the AP
        // to let the user read the confirmation message on their phone.
        // Otherwise, the AP would disappear immediately after saving, which can be confusing.
        Serial.println("WiFi saved. Keeping AP alive for 30s...");
        delay(30000);
        setRingRgb(0, 0, 0); // Off to indicate the process is complete and AP is going away
    });

    String customHead = WIFI_MANAGER_CUSTOM_HEAD;
    customHead.replace("%HOSTNAME%", hostname);
    wm.setCustomHeadElement(customHead.c_str());

    String apName = "Led'o'clock AP " + chipId;
    // Run the config portal in non-blocking mode so the main firmware keeps
    // executing while the Access Point is active. This allows the long-press
    // factory reset button to be polled while the device is in AP mode.
    wm.setConfigPortalBlocking(false);
    if (!wm.autoConnect(apName.c_str())) {
        // Not connected yet: the config portal (AP mode) is running.
        // Keep servicing the portal and watch for a long button press.
        while (WiFi.status() != WL_CONNECTED) {
            wm.process();
            checkFactoryResetButton();

            // If the config portal is no longer active (e.g. it timed out)
            // without a successful connection, restart to retry, matching the
            // previous blocking behaviour.
            if (!wm.getConfigPortalActive() && WiFi.status() != WL_CONNECTED) {
                Serial.println("Config portal closed without connection. Restarting...");
                ESP.restart();
            }

            delay(10);
        }
    }

    configureWiFiRoaming();

    Serial.println("Connected to WiFi. IP address: " + WiFi.localIP().toString());

    showWifiSuccessFeedback();

    // --- Preferences Initialization ---
    initPreferences();
    
    globalBrightness = loadBrightness();

    loadColorsFromFS();
    int savedColorIndex = loadColorModeIndex();
    String initialColor = "off"; 
    if (savedColorIndex < definedColorCount) {
        initialColor = definedColors[savedColorIndex].name;
    }
    applyColor(initialColor);

    loadSchedulesFromFS();
    loadTimezoneFromFS();

    Serial.println("Initialization complete.");

    // Restore the initial color based on EEPROM or schedule
    applyColor(initialColor);

    if (MDNS.begin(hostname.c_str())) {
        Serial.println("MDNS responder started with hostname: " + hostname);
    }

    // Time Sync with saved timezone
    configTzTime(currentTimezone.c_str(), "pool.ntp.org");

    initEndpoints();
}

void loop() {
    handleWebClient();

    // Check WiFi Reset Button (long press)
    checkFactoryResetButton();

    // Flag to track if we have applied the correct schedule after a reboot
    static bool initialTimeSyncDone = false;
    static unsigned long lastCheck = 0;

    if (millis() - lastCheck > 1000) {
        // If time is not synced yet, check if it just became available
        if (!initialTimeSyncDone) {
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            // If year is >= 100, the ESP has received the NTP time
            if (p_tm->tm_year >= 100) {
                Serial.println("Time synchronized via NTP.");
                Serial.println("Evaluating correct schedule...");
                
                initialTimeSyncDone = true;
                
                // Force the color calculation based on the new absolute time
                forceStateEvaluation();
            }
        }

        checkTimeEvents();
        updateLedRing();
        lastCheck = millis();
    }
}
