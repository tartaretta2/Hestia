#pragma once

#ifndef SIM
void initAlarmLED(const char* gpioChip, const unsigned int ledPin);
void toggleAlarmLED(const unsigned int ledPin);

void initLightsLED(const char* gpioChip, const unsigned int ledPin);
void setLED(const unsigned int ledPin);

void cleanupLEDs();
#endif

void simulateLED();