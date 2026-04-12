#ifndef OTA_SERVICE_H
#define OTA_SERVICE_H

#include <Arduino.h>

struct FirmwareInfo {
    String latestVersion;
    String downloadUrl;
};

FirmwareInfo getLatestFirmwareInfo();
void updateFirmwareIfNeeded();

#endif