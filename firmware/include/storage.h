#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>

// --- LittleFS File Storage ---
void loadColorsFromFS();
void saveColorsToFS(String json);
void saveSchedulesToFS(String json);
void loadSchedulesFromFS();
void saveTimezoneToFS(String tz);
void loadTimezoneFromFS();
void cleanupStorage();

// --- Full Reset ---
void performFactoryReset();

// --- Preferences (NVS) Storage ---
void initPreferences();
void saveBrightness(int brightness);
int loadBrightness();
void saveColorModeIndex(int index);
int loadColorModeIndex();
void clearPreferences();

#endif