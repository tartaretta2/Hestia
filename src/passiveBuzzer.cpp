#include "buzzer.h"
#include <iostream>
#include <thread>
#include <chrono>

#ifndef SIM
    #include <pigpio.h>
#endif

using namespace std;

#ifndef SIM

void initBuzzer(const char* GPIO_CHIP, const unsigned int BUZZER_PIN){
    if (gpioInitialise() < 0) {
        cerr << "Failed to initialise pigpio" << endl;
        return;
    }
    gpioSetMode(BUZZER_PIN, PI_OUTPUT);
    gpioWrite(BUZZER_PIN, 0);
}

void playTone(const unsigned int BUZZER_PIN, int frequency, int duration){
    gpioHardwarePWM(BUZZER_PIN, frequency, 500000); // 50% duty cycle
    this_thread::sleep_for(chrono::milliseconds(duration));
    gpioWrite(BUZZER_PIN, 0);
}

void toggleBuzzer(const unsigned int BUZZER_PIN){
    int freq;
    for(freq = 800; freq <= 1200; freq += 10)
        playTone(BUZZER_PIN, freq, 20);
    for(freq = 1200; freq >= 800; freq -= 10)
        playTone(BUZZER_PIN, freq, 20);
}

void cleanupBuzzer(){
    gpioWrite(BUZZER_PIN, 0);
    gpioTerminate();
}

#endif

void simulateBuzzer(){
    cout << "BUZZER ON" << endl;
}