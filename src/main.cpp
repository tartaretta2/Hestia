#include "motion_sensor.h"
#include "led.h"
#include "buzzer.h"
#include <iostream>
#include <thread>
#include <chrono>

#ifndef SIM
    #define GPIO_CHIP "/dev/gpiochp4"
    #define MS_PIN 17
    #define BUZZER_PIN 27
    #define LED_PIN 22
#endif

using namespace std;

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
        toggleLED();
        toggleBuzzer();
    #endif
}

int main(){
    #ifndef SIM
        initMS(GPIO_CHIP, MS_PIN);
        //initLED(LED_PIN);
        //initBuzzer(BUZZER_PIN);
    #endif

    int i, sensorRead;;
    for(i = 0; i < 10; ++i){
        sensorRead = readSensor();
        cout << "Lettura " << i + 1 << ": " << (sensorRead > 3 ? "Motion detected" : "No Motion") << endl;
        
        if(sensorRead > 3)   
            alarm(); 
        
        this_thread::sleep_for(chrono::seconds(1));
    }
    return 0;
}
