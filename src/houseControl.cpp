#include "houseControl.h"
#include "motion_sensor.h"
#include "led.h"
#include "buzzer.h"
#include "ir_remote.h"
#include "gate.h"
#include "cameraYOLO.h"
#include "Dht11.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>

#ifndef SIM
    #include <gpiod.h>
#endif

static const vector<string> authorizedPlates = {
    "CZ889KF"
};

// shared state atomic (thread-safe) variables
extern atomic<bool> sirenOn;
extern atomic<bool> alarmOn;
extern atomic<bool> running;
extern atomic<bool> lightsOn;
extern atomic<bool> gateOpen;
extern atomic<bool> disarmRequested; //set by plate recognition 
extern atomic<bool> armRequested; //set by web interface 
extern atomic<bool> lightsManualMode;
extern atomic<bool> acManualMode; 
extern atomic<bool> heatingManualMode; 
extern atomic<bool> acOn; 
extern atomic<bool> heatingOn; 

static thread alarmMSthread;   // handles alarm motion-sensor edge events
static thread lightsThread;    // handles lights motion-sensor edge events
static thread tempThread;      // polls DHT11 periodically 
static atomic<uint64_t> lightsOffToken(0); // token used to check whether the thread should turn off lights

void toggleAlarmActivation() {
     // Reset the arm/disarm request flags
    armRequested = false; 
    disarmRequested = false;
    
    #ifdef SIM
        alarmOn = !alarmOn;
        cout << (alarmOn ? "Alarm ON" : "Alarm OFF") << endl;
    #else
    if (!alarmOn) {
        cout << "[Alarm] You have 5 seconds to leave the house before the alarm system starts." << endl;

        for (int i = 5; i > 0; --i) {
            cout << i << "... " << endl;
            this_thread::sleep_for(chrono::seconds(1));
        }
        cout << endl;
        
        this_thread::sleep_for(chrono::seconds(10)); //additional wait to ensure motion sensor is stable on low before starting the listener and avoid false triggers
        
        //start the camera for plate recognition
        initCameraSystem();

        if (!initAlarmMS(GPIO_CHIP, ALARM_MS)) {
            cerr << "Unable to arm alarm: motion sensor init failed." << endl;
            return;
        }

        alarmOn = true; 

        // start the thread that blocks until a motion edge arrives
        alarmMSthread = thread(alarmMSListener);

    } else {
        // disarm the alarm system: stop motion monitoring 
        alarmOn = false;
        sirenOn = false;

        if (alarmMSthread.joinable()) alarmMSthread.join();

        cleanupAlarmMS();      
        stopCameraSystem();

        cout << "Alarm OFF" << endl;
    }
    #endif
}

void toggleSiren() {
    // Only allow siren changes when alarm is armed
    if (!alarmOn) return;

    sirenOn = !sirenOn;
    cout << "Siren " << (sirenOn ? "ON" : "OFF") << endl;
}

void alarmMSListener() {
    cout << "Alarm ON" << endl;
    
    while (alarmOn) {
        try {
#ifdef SIM
            if (simulateMS())
#else
            if (readAlarmMS())
#endif
            {
                if (!sirenOn) {
                    cout << "Motion detected! Triggering siren" << endl;
                    toggleSiren();
                }
            }
        } catch (const exception& e) {
            cerr << "Error reading motion sensor: " << e.what() << endl;
        }
    }

    cout << "Motion sensor listener stopped" << endl;
}

void lightsMSListener(bool firstEntry) {

    cout << "Lights motion sensor listener started." << endl;

    while (running) {
        try {
            if(lightsManualMode) return; // If manual mode is active quit the listener

        #ifdef SIM
            if (simulateMS()) {
                if (!lightsOn) {
                    simulateLED(true);
                    lightsOn = true;
                    cout << "LightsMS: motion detected, lights ON" << endl;
                }
                // update the token to cancel any pending lights-off 
                lightsOffToken.fetch_add(1);
            } else {
                if (lightsOn) {
                    // Simulate the falling edge / inactivity timer
                    auto token = ++lightsOffToken;
                    thread([token]() {
                        this_thread::sleep_for(chrono::seconds(10));
                        if (lightsManualMode) return; // Skip motion sensor control if mode is manual
                        if (token == lightsOffToken.load()) { // only turn off lights if no new motion has been detected since the thread sleep started
                            simulateLED(false);
                            lightsOn = false;
                        }
                    }).detach();
                    cout << "LightsMS: motion ended, lights OFF scheduled" << endl;
                }
            }
            this_thread::sleep_for(chrono::seconds(2)); // in simulation check every 2 sec
#else
            if (firstEntry) {
                this_thread::sleep_for(chrono::seconds(3)); // Wait a bit before starting to avoid false triggers on startup
                firstEntry = false;
            }

            int edge = readLightsMS();

            if (edge > 0) {
                // Rising edge: motion detected
                if (!lightsOn) {
                    setLED(LIGHTS_LED, true);
                    lightsOn = true;
                    cout << "LightsMS: motion detected, lights ON" << endl;
                }
                lightsOffToken.fetch_add(1);
            } else if (edge < 0) {
                // Falling edge: motion ended, schedule lights off
                if (lightsOn) {
                    auto token = ++lightsOffToken;
                    cout << "LightsMS: falling edge, lights OFF scheduled" << endl;
                    thread([token]() {
                        this_thread::sleep_for(chrono::seconds(3));
                        if (lightsManualMode) return; 
                        if (token == lightsOffToken.load()) {
                            setLED(LIGHTS_LED, false);
                            lightsOn = false;
                        }
                    }).detach();
                }
            }
#endif
        } catch (const exception& e) {
            cerr << "Error in lights motion sensor: " << e.what() << endl;
        }
    }

    #ifndef SIM
    // Ensure lights are off on exit
    if (lightsOn) {
        setLED(LIGHTS_LED, false);
    }
    #endif

    cout << "Lights motion sensor listener stopped" << endl;
}

void startLightsListener() {
    if (lightsThread.joinable()) return; // already running
    lightsThread = thread(lightsMSListener, true);
}

void toggleGateActivation() {
    #ifdef SIM
        simulateGate();
    #else
        toggleGate(GATE_PIN);
    #endif
}

void toggleLightsActivation() {
    
    lightsOn = !lightsOn;

    #ifdef SIM
        simulateLED(lightsOn);
    #else
        setLED(LIGHTS_LED, lightsOn);
    #endif
    cout << (lightsOn ? "Lights ON" : "Lights OFF") << endl;
}

void shutdownSystem(){
    // Request main loop and all listeners to exit
    running = false;

    // Wait for all listener threads to cleanly exit
    if (lightsThread.joinable()) lightsThread.join();

    if (tempThread.joinable()) tempThread.join();

    #ifndef SIM
    // Make sure siren is turned off first, then disable the alarm
    if (sirenOn) toggleSiren();
    if (alarmOn) toggleAlarmActivation();

    cleanupLEDs();
    cleanupBuzzer(BUZZER_PIN);
    cleanupGate(GATE_PIN);
    cleanupDHT11();
    cleanupIR();
    cleanupAlarmMS();
    cleanupLightsMS();
    #endif
}


void checkPlate(const string& plate) {
    for (const auto& p : authorizedPlates) {
        if (plate == p) {
            cout << "Authorized plate recognized: " << plate << endl;

            if (alarmOn) {
                cout << "Disarm requested by recognized plate" << endl;
                disarmRequested = true;

                toggleGateActivation();
                this_thread::sleep_for(chrono::seconds(10));
                toggleGateActivation();
            }
            return;
        }
    }
    cout << "Unauthorized plate detected: " << plate << endl;
}

void toggleLightsMode(){

    lightsManualMode = (!lightsManualMode);
    cout << "[Lights] Lights mode is now " << (lightsManualMode ? "MANUAL" : "AUTO") << endl;

    #ifndef SIM
    if (lightsManualMode){
        if (lightsThread.joinable()) lightsThread.join();
        cleanupLightsMS();
    }else{
        initLightsMS(GPIO_CHIP, LIGHTS_MS);
        startLightsListener();
    }
    #endif
}

void toggleACMode() {
    acManualMode = !acManualMode;
    cout << "[AC] AC mode is now " << (acManualMode ? "MANUAL" : "AUTO") << endl;
}

void toggleHeatingMode() {
    heatingManualMode = !heatingManualMode;
    cout << "[HEATING] Heating mode is now " << (heatingManualMode ? "MANUAL" : "AUTO") << endl;
}

void toggleAC() {
    acOn = !acOn;
    cout << "[AC] AC is now " << (acOn ? "ON" : "OFF") << endl;
    #ifndef SIM
        setLED(AC_LED, acOn);
    #else
        simulateLED(acOn);
    #endif
}

void toggleHeating() {
    heatingOn = !heatingOn;
    cout << "[HEATING] Heating is now " << (heatingOn ? "ON" : "OFF") << endl;
    #ifndef SIM
        setLED(HEATING_LED, heatingOn);
    #else
        simulateLED(heatingOn);
    #endif
}

// Reads DHT11 periodically and triggers the AC/heating LEDs when threshold are crossed
void temperatureMonitor()
{
    while (running) {
        float temp = 0.0f, hum = 0.0f;
        if (readDHT11(temp, hum)) {
            if(acManualMode && heatingManualMode) {
                this_thread::sleep_for(chrono::seconds(3));
                continue;
            }

            if(!acManualMode){
                // cout << "[DHT11] Temp: " << temp << " C  |  Hum: " << hum << " %" << endl;
                bool acShouldBeOn = (temp > AC_THRESHOLD_C);
                if (acShouldBeOn && !acOn) {
                    // transition from cool to hot (turn the AC on)
                    cout << "[AC] Temperature: " << temp << " C | Humidity: " << hum << " %" << endl;
                    toggleAC();
                } else if (!acShouldBeOn && acOn) {
                    // transition from hot to cool (turn the AC off)
                    cout << "[AC] Temperature: " << temp << " C | Humidity: " << hum << " %" << endl;
                    toggleAC();
                }
            }

            if(!heatingManualMode){
                bool heatingShouldBeOn = (temp < HEATING_THRESHOLD_C);
                if (heatingShouldBeOn && !heatingOn) {
                    // transition from hot to cool (turn the heating on)
                    cout << "[HEATING] Temperature: " << temp << " C | Humidity: " << hum << " %" << endl;
                    toggleHeating();
                } else if (!heatingShouldBeOn && heatingOn) {
                    // transition from cool to hot (turn the heating off)
                    cout << "[HEATING] Temperature: " << temp << " C | Humidity: " << hum << " %" << endl;
                    toggleHeating();
                }
            }

        } else {
            #ifdef SIM
                cout << "[DHT11] Temp: " <<rand()%(5) + 28 << " C  |  Hum: " << rand()%(50) + 30 << " %" << endl;
            #else
                cout << "[DHT11] Read failed." << endl;
            #endif
        }
        this_thread::sleep_for(chrono::seconds(3));
    }
}

void startTemperatureMonitor() {
    tempThread = thread(temperatureMonitor);
}
