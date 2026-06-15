#pragma once

#ifndef SIM
void initBuzzer(const char* GPIO_CHIP, const unsigned int BUZZER_PIN);
void toggleBuzzer(const unsigned int BUZZER_PIN);
void cleanupBuzzer(const unsigned int BUZZER_PIN);
#endif 

void simulateBuzzer();
