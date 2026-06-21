#pragma once

#ifndef SIM
bool initAlarmMS(const char* gpioChip, const unsigned int msPin);
int readAlarmMS();
bool isAlarmMSActive();

bool initLightsMS(const char* gpioChip, const unsigned int msPin);
int readLightsMS();

void cleanupMSs();
#endif

int simulateMS();