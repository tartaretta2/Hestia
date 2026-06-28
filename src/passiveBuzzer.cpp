#include "buzzer.h"
#include "houseControl.h"
#include <iostream>
#include <thread>
#include <chrono>

#ifndef SIM
    #include <lgpio.h>
#endif

using namespace std;

#ifndef SIM
static int handle = -1;

void initBuzzer(const char* gpioChip, const unsigned int buzzerPin){
    handle = lgGpiochipOpen(4);
    cout << "Initializing BUZZER on pin: " << buzzerPin << endl;
    if (handle < 0) {
        cerr << "Failed to open GPIO chip" << endl;
        return;
    }
    lgGpioClaimOutput(handle, 0, buzzerPin, 0);
}

// siren
void toggleBuzzer(const unsigned int buzzerPin){
    int freq;
    for(freq = 800; freq <= 1200; freq += 10){
        lgTxPwm(handle, buzzerPin, freq, 50, 0, 0);
        this_thread::sleep_for(chrono::milliseconds(20));
    }
        
    for(freq = 1200; freq >= 800; freq -= 10){
        lgTxPwm(handle, buzzerPin, freq, 50, 0, 0);
        this_thread::sleep_for(chrono::milliseconds(20));
    }

    lgTxPwm(handle, buzzerPin, 0, 0, 0, 0);
}

void cleanupBuzzer(const unsigned int buzzerPin){
    lgGpioWrite(handle, buzzerPin, 0);
    lgGpiochipClose(handle);
}

#endif

void simulateBuzzer(){
    cout << "BUZZER ON" << endl;
}