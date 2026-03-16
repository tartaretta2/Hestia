#include "buzzer.h"
#include <iostream>
#include <thread>
#include <chrono>

#ifndef SIM
    #include <lgpio.h>
#endif

using namespace std;

#ifndef SIM
static int handle = -1;

void initBuzzer(const char* GPIO_CHIP, const unsigned int BUZZER_PIN){
    handle = lgGpiochipOpen(4);
    cout << "Initializing BUZZER on pin: " << BUZZER_PIN << endl;
    if (handle < 0) {
        std::cerr << "Failed to open GPIO chip" << std::endl;
        return;
    }
    lgGpioClaimOutput(handle, 0, BUZZER_PIN, 0);
}

void toggleBuzzer(const unsigned int BUZZER_PIN){
    int freq;
    for(freq = 800; freq <= 1200; freq += 10){
        lgTxPwm(handle, BUZZER_PIN, freq, 50, 0, 0);
        this_thread::sleep_for(chrono::milliseconds(20));
    }
        
    for(freq = 1200; freq >= 800; freq -= 10){
        lgTxPwm(handle, BUZZER_PIN, freq, 50, 0, 0);
        this_thread::sleep_for(chrono::milliseconds(20));
    }

    lgTxPwm(handle, BUZZER_PIN, 0, 0, 0, 0);
}

void cleanupBuzzer(const unsigned int BUZZER_PIN){
    lgGpioWrite(handle, BUZZER_PIN, 0);
    lgGpiochipClose(handle);
}

#endif

void simulateBuzzer(){
    cout << "BUZZER ON" << endl;
}