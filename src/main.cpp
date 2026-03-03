#include "motion_sensor.h"
#include "led.h"
#include "buzzer.h"
#include <iostream>
#include <thread>
#include <chrono>

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
