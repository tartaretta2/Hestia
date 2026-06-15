#pragma once

#ifndef SIM
void initLED(const char* GPIO_CHIP, const unsigned int LED_PIN);
void toggleLED(const unsigned int LED_PIN);
void cleanupLED();
#endif

void simulateLED();