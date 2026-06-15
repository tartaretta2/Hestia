#pragma once

#ifndef SIM
void initLED(const char* gpioChip, const unsigned int ledPin);
void toggleLED(const unsigned int ledPin);
void cleanupLED();
#endif

void simulateLED();