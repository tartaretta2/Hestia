#include "motion_sensor.h"
#include <iostream>

using namespace std;

int simulateMS(){
    //Motion sensor simulation: > 3 means motion detected
    return rand() % 5;
}

int readMS(){
    return -1;
}