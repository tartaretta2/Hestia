#include "buzzer.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

void simulateBuzzer(){
    //Buzzer simulation: turn on buzzer for 5 seconds
    int i;
    for(i = 0; i < 5; ++i){
        cout << "BUZZER ON" << endl;
        this_thread::sleep_for(chrono::milliseconds(500));
    }
}

void toggleBuzzer(){
    
}