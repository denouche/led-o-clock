#include "led_control.h"
#include "storage.h"
#include <time.h>

static Adafruit_NeoPixel ring(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

/**
 * Internal helper to set all pixels to a single color and refresh the ring.
 * Centralizes the setPixelColor loop and ring.show().
 */
static void updatePixels(uint32_t color) {
    ring.setBrightness(globalBrightness);
    
    for (int i = 0; i < NUM_LEDS; i++) {
        ring.setPixelColor(i, color);
    }
    
    ring.show();
}

/**
 * Public function to set the entire ring using raw RGB values.
 */
void setRingRgb(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = ring.Color(r, g, b);
    updatePixels(color);
}

void setBrightness(int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    
    globalBrightness = (percent * 255 + 50) / 100; // Convert 0-100% to 0-255 with rounding
    ring.setBrightness(globalBrightness);
    ring.show();
}

/**
 * Helper to find the RGB value of a defined color by its name.
 * Returns black (off) if not found.
 */
static uint32_t getRgbFromName(String colorName) {
    for (int i = 0; i < definedColorCount; i++) {
        if (definedColors[i].name == colorName) {
            return ring.Color(definedColors[i].r, definedColors[i].g, definedColors[i].b);
        }
    }
    
    return ring.Color(0, 0, 0);
}

/**
 * Renders a full solid color on the ring based on a name.
 */
void renderSolidColor(String color) {
    uint32_t rgbColor = getRgbFromName(color);
    updatePixels(rgbColor);
}

/**
 * Triggers a color change and persists it in Preferences.
 */
void applyColor(String color) {
    Serial.println("Applying color: " + color);
    
    currentColorMode = color;
    
    // Reset the LED count tracker when switching modes
    currentLedsOn = NUM_LEDS; 
    
    // Draw the full color initially
    renderSolidColor(color);

    // Persist color mode in Preferences
    int colorCode = 255;
    for (int i = 0; i < definedColorCount; i++) {
        if (definedColors[i].name == color) {
            colorCode = i;
            break;
        }
    }

    if (loadColorModeIndex() != colorCode) {
        saveColorModeIndex(colorCode);
    }
}

/**
 * Handles the countdown logic: turns off LEDs one by one 
 * based on time progress between schedules.
 */
void updateLedRing() {
    if (currentColorMode == "off" || currentColorMode == "") {
        return;
    }

    time_t now = time(nullptr);
    struct tm* p_tm = localtime(&now);

    if (p_tm->tm_year < 100 || scheduleCount == 0) {
        return;
    }

    // Map time: tm_wday is 0 for Sunday. We map it to 0 for Monday to match our array.
    int currentDayIndex = (p_tm->tm_wday + 6) % 7; 
    
    // Convert current time to seconds from the start of the week for smooth animation
    long nowSec = (currentDayIndex * 24 * 3600) + (p_tm->tm_hour * 3600) + (p_tm->tm_min * 60) + p_tm->tm_sec;
    
    int activeIndex = -1;
    long minElapsed = 604801;
    long minRemaining = 604801;

    for (int i = 0; i < scheduleCount; i++) {
        for (int d = 0; d < 7; d++) {
            if (schedules[i].days[d]) {
                long schedSec = (d * 24 * 3600) + (schedules[i].hour * 3600) + (schedules[i].minute * 60);
                long elapsed = nowSec - schedSec;
                
                if (elapsed < 0) {
                    elapsed += 604800;
                }

                if (elapsed < minElapsed) {
                    minElapsed = elapsed;
                    activeIndex = i;
                }

                // Calculate how much time remains until this schedule event happens again
                long remaining = schedSec - nowSec;
                
                // If remaining is exactly 0, it happens now. We wrap to 7 days to find the NEXT event.
                if (remaining <= 0) {
                    remaining += 604800;
                }
                
                if (remaining < minRemaining) {
                    minRemaining = remaining;
                }
            }
        }
    }

    if (activeIndex == -1) {
        return;
    }

    // If countdown is disabled, ensure all LEDs are on
    if (!schedules[activeIndex].countdown) {
        if (currentLedsOn != NUM_LEDS) {
            renderSolidColor(currentColorMode);
            currentLedsOn = NUM_LEDS;
        }
        
        return;
    }

    // --- Countdown Calculation ---
    // The total time between the active schedule and the next is exactly elapsed + remaining
    float totalDuration = (float)(minElapsed + minRemaining);
    float progress = (totalDuration > 0) ? (float)minElapsed / totalDuration : 0.0;

    int ledsToKeepOn = NUM_LEDS - (int)(progress * NUM_LEDS);
    
    if (ledsToKeepOn < 1) {
        ledsToKeepOn = 1;
    }

    // Update only if the number of active LEDs has changed
    if (ledsToKeepOn != currentLedsOn) {
        currentLedsOn = ledsToKeepOn;
        uint32_t activeRgbColor = getRgbFromName(currentColorMode);
        
        ring.setBrightness(globalBrightness);
        
        for (int i = 0; i < NUM_LEDS; i++) {
            if (i < ledsToKeepOn) {
                ring.setPixelColor(i, activeRgbColor);
            } else {
                ring.setPixelColor(i, ring.Color(0, 0, 0));
            }
        }
        
        ring.show();
    }
}

/**
 * Evaluates the current time against schedules to set the initial color.
 */
void forceStateEvaluation() {
    if (scheduleCount == 0) {
        applyColor("off");
        return;
    }

    time_t now = time(nullptr);
    struct tm* p_tm = localtime(&now);
    
    if (p_tm->tm_year < 100) {
        return; 
    }

    int currentDayIndex = (p_tm->tm_wday + 6) % 7;
    int nowMin = (currentDayIndex * 24 * 60) + (p_tm->tm_hour * 60) + p_tm->tm_min;
    
    int activeIndex = -1;
    int minElapsed = 10081;

    for (int i = 0; i < scheduleCount; i++) {
        for (int d = 0; d < 7; d++) {
            if (schedules[i].days[d]) {
                int schedMin = (d * 24 * 60) + (schedules[i].hour * 60) + schedules[i].minute;
                int elapsed = nowMin - schedMin;
                
                if (elapsed < 0) {
                    elapsed += 10080;
                }

                if (elapsed < minElapsed) {
                    minElapsed = elapsed;
                    activeIndex = i;
                }
            }
        }
    }

    if (activeIndex != -1) {
        applyColor(schedules[activeIndex].color);
    } else {
        applyColor("off");
    }
}

/**
 * Periodically checks for schedule triggers.
 */
void checkTimeEvents() {
    static int currentMinute = -1;

    time_t now = time(nullptr);
    struct tm* p_tm = localtime(&now);

    if (p_tm->tm_year < 100) {
        return; 
    }

    if (p_tm->tm_min != currentMinute) {
        currentMinute = p_tm->tm_min;
        
        int currentDayIndex = (p_tm->tm_wday + 6) % 7;
        
        for (int i = 0; i < scheduleCount; i++) {
            if (schedules[i].active && 
                schedules[i].days[currentDayIndex] && 
                schedules[i].hour == p_tm->tm_hour && 
                schedules[i].minute == p_tm->tm_min) {
                
                applyColor(schedules[i].color);
            }
        }
    }
}

void initLedRing() {
    ring.begin();
    ring.show();
}

/**
 * Visual feedback for factory reset.
 */
void showFactoryResetFeedback() {
    setRingRgb(255, 0, 0); // Red
}

/**
 * Visual feedback for firmware update.
 */
void showFirmwareUpdateFeedback() {
    setRingRgb(200, 0, 200); // Purple light
}
