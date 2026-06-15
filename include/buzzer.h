#pragma once

#ifndef SIM
void initBuzzer(const char* gpioChip, const unsigned int buzzerPin);
void toggleBuzzer(const unsigned int buzzerPin);
void cleanupBuzzer(const unsigned int buzzerPin);
#endif 

void simulateBuzzer();