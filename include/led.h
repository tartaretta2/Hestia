#pragma once

#ifndef SIM
void initAlarmLED(const char* gpioChip, const unsigned int ledPin);
void toggleAlarmLED(const unsigned int ledPin);

void initTempLED(const char* gpioChip, const unsigned int ledPin);

void initLightsLED(const char* gpioChip, const unsigned int ledPin);

void setLED(const unsigned int ledPin, bool on);

void cleanupLEDs();
#endif

void simulateLED(bool on);