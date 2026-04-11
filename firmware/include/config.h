#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- Device Configuration ---
const int PIN_BUTTON = 9;   // GPIO9 used for Flash Mode and WiFi Reset
const int LED_PIN    = 3;   // WS2812B Data pin
const int NUM_LEDS   = 12;  // Adjust this to match your ring size (12, 16, 24...)

// --- Constants ---
const int MAX_COLORS = 8;
const int MAX_SCHEDULES = 12;

// --- Structures ---
struct ColorDef {
    String name;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct Schedule {
    int hour;
    int minute;
    String color;
    bool active;
    bool countdown;
    bool days[7]; // active days: 0 = Mon, 1 = Tue, 2 = Wed, 3 = Thu, 4 = Fri, 5 = Sat, 6 = Sun
};

// --- Global Variables (Externs) ---
extern ColorDef definedColors[MAX_COLORS];
extern int definedColorCount;

extern Schedule schedules[MAX_SCHEDULES];
extern int scheduleCount;

extern int globalBrightness;
extern String currentColorMode;
extern int currentLedsOn;

extern String currentTimezone;
extern const char* FIRMWARE_VERSION;

#endif