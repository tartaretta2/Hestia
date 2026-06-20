#pragma once

#ifndef SIM
void initGate(const char* gpioChip, const unsigned int gatePin);
void toggleGate(const unsigned int gatePin);
void cleanupGate(const unsigned int gatePin);
#endif

void simulateGate();