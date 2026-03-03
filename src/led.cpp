#include "led.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

void simulateLED(){
    //LED simulation: turn on LED for 5 seconds
    int i;
    for(i = 0; i < 5; ++i){
        cout << "LED ON" << endl;
        this_thread::sleep_for(chrono::milliseconds(500));
    }
}

void toggleLED(){

}