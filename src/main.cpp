#include "motion_sensor.h"
#include "led.h"
#include "buzzer.h"
#include <iostream>
#include <thread>
#include <atomic>

#ifndef SIM
    #define GPIO_CHIP "/dev/gpiochip4"
    #define MS_PIN 17
    #define BUZZER_PIN 27
    #define LED_PIN 22
#endif

using namespace std;

atomic<bool> running(true);

int readSensor(){
    #ifdef SIM
        return simulateMS();
    #else
        return readMS();
    #endif
}

void alarm(){
    #ifdef SIM
        simulateLED();
        simulateBuzzer();
    #else
        simulateLED();
        simulateBuzzer();
        //toggleLED();
        //toggleBuzzer();
    #endif
}

void MSListener(){
    initMS(GPIO_CHIP, MS_PIN);
    
    cout << "Motion Sensor Listener started..." << endl;
    while(running){
        if(readSensor()){
            cout << "Motion detected! Triggering alarm..." << endl;
            alarm();
        }
    }
}

int main(){
    
    thread listener(MSListener);

    cout << "System running. Press Enter to stop..." << endl;
    cin.get();
    running = false;
    listener.join();

    cout << "Exiting..." << endl;

    return 0;
}
