#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Adafruit_NeoPixel.h>
#include "config.h"

void setRingRgb(uint8_t r, uint8_t g, uint8_t b);
void setBrightness(int percent);

void renderSolidColor(String color);
void applyColor(String color);
void updateLedRing();
void forceStateEvaluation();
void checkTimeEvents();
void initLedRing();
void showFactoryResetFeedback();

#endif