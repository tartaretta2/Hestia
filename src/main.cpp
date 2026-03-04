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
    int i;
    for(i = 0; i < 5; ++i){
        #ifndef SIM
            toggleLED(LED_PIN);
            toggleBuzzer(BUZZER_PIN);
            this_thread::sleep_for(chrono::milliseconds(500));
        #else
            simulateLED();
            simulateBuzzer();   
        #endif
    }
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
    #ifndef SIM
        initBuzzer(GPIO_CHIP, BUZZER_PIN);
        initLED(GPIO_CHIP, LED_PIN);
    #endif

    thread listener(MSListener);

    cout << "System running. Press Enter to stop..." << endl;
    cin.get();
    running = false;
    listener.join();

    #ifndef SIM
        cleanupBuzzer();
        cleanupLED();
    #endif
   
    cout << "System shut down." << endl;

    return 0;
}
