#pragma once

#ifndef SIM
    void initMS(const char* gpioChip, const unsigned int msPin);
    int readMS();
    void cleanupMS();
#endif

int simulateMS();