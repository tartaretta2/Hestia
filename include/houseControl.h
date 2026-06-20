#pragma once
#include "ir_sensor.h"
#include <atomic>

#ifndef SIM
    #define GPIO_CHIP "/dev/gpiochip4"
    #define MS_PIN 17
    #define BUZZER_PIN 27
    #define LED_PIN 22
    #define IR_PIN 25
    #define GATE_PIN 24                
#endif

void toggleAlarmActivation();
void toggleGateActivation();
void MSListener();
void toggleSiren();
void shutdownSystem();
void checkPlate(const std::string& plate);