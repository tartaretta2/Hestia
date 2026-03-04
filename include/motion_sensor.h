#pragma once

#ifndef SIM
    void initMS(const char* GPIO_CHIP, int MS_PIN);
    int readMS();
#endif

int simulateMS();
