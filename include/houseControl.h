#pragma once
#include "ir_sensor.h"
#include <atomic>
#include <string>

#ifndef SIM
    #define GPIO_CHIP "/dev/gpiochip4"
    // GPIO pin numbers for the various devices

    //alarm system
    #define ALARM_MS 17
    #define BUZZER_PIN 27
    #define ALARM_LED 22
    
    //lights
    #define LIGHTS_MS 26
    #define LIGHTS_LED 16

    //IR receiver
    #define IR_PIN 25
    
    //gate
    #define GATE_PIN 24   
    
    //DHT11
    #define AC_LED 23
    #define HEATING_LED 5
#endif

void toggleAlarmActivation();
void toggleGateActivation();
void toggleLightsActivation();
void alarmMSListener();
void lightsMSListener(bool firstEntry = true);
void startLightsListener();
void toggleSiren();
void shutdownSystem();
void checkPlate(const std::string& plate);
void toggleLightsMode();
void toggleACMode();
void toggleHeatingMode(); 
void toggleAC();
void toggleHeating();