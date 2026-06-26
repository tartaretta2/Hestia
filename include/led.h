#pragma once

#ifndef SIM
   
#include <gpiod.h>

void initOneLED(const char* gpioChip, unsigned int ledPin, const char* consumer, gpiod_chip** chip_out, gpiod_line_request** req_out);

void initLEDs(const char* gpioChip, unsigned int alarmPin, unsigned int lightsPin, unsigned int acPin, unsigned int heatingPin);

void toggleAlarmLED(const unsigned int ledPin);

void setLED(const unsigned int ledPin, bool on);

void cleanupLEDs();

#endif

void simulateLED(bool on);