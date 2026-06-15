#pragma once

#ifndef SIM
    void initMS(const char* GPIO_CHIP, const unsigned int MS_PIN);
    int readMS();
#endif

int simulateMS();
