#pragma once

using namespace std;

#ifndef SIM
bool initAlarmMS(const char* gpioChip, const unsigned int msPin);
int readAlarmMS();

bool initLightsMS(const char* gpioChip, const unsigned int msPin);
int readLightsMS();

void cleanupAlarmMS();
void cleanupLightsMS();
#endif

int simulateMS();