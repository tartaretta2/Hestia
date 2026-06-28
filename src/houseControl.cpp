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

using namespace std;

static const vector<string> authorizedPlates = {
    "CZ889KF"
};

// Shared state variables defined in main.cpp
extern atomic<bool> sirenOn;
extern atomic<bool> alarmOn;
extern atomic<bool> running;
extern atomic<bool> lightsOn;
extern atomic<bool> gateOpen;
extern atomic<bool> disarmRequested; //flag set by plate recognition to request disarming the alarm system (checked in main loop)
extern atomic<bool> armRequested; //flag set by web command to request arming the alarm system (checked in main loop)
extern atomic<bool> lightsManualMode;
extern atomic<bool> acManualMode; // true when the AC is in manual mode (set by web command)
extern atomic<bool> heatingManualMode; // true when the heating is in manual mode (set by web command)
extern atomic<bool> acOn; // true when the AC is on (set by temperature monitor thread)
extern atomic<bool> heatingOn; // true when the Heating is on (set by temperature monitor thread)

// Threads used for asynchronous tasks
static thread alarmMSthread;   // thread that blocks on alarm motion-sensor edge events
static thread lightsThread;    // thread that blocks on lights motion-sensor edge events
static atomic<uint64_t> lightsOffToken(0); // token used to check if thread should turn off lights

void toggleAlarmActivation() {
    armRequested = false; // Reset the arm request flag
    disarmRequested = false; // Reset the disarm request flag
    
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
        //Start the camera for plate recognition
              
        this_thread::sleep_for(chrono::seconds(10)); // Additional wait to ensure motion sensor is stable low before starting the listener and avoid false triggers.

        initCameraSystem();
        // Arm the alarm system: create the GPIO request and start the listener.
        if (!initAlarmMS(GPIO_CHIP, ALARM_MS)) {
            cerr << "Unable to arm alarm: motion sensor init failed." << endl;
            return;
        }

        // If the PIR is already active when arming, trigger immediately so
        // we do not miss the first state after a disarm/re-arm cycle.
        // if (isAlarmMSActive() && !sirenOn) {
        //     cout << "Motion already active, triggering siren immediately." << endl;
        //     toggleSiren();
        // }
        alarmOn = true; 

        // Start the thread that blocks until a motion edge arrives.
        alarmMSthread = thread(alarmMSListener);

    } else {
        // Disarm the alarm system: stop motion monitoring and release the GPIO request.
        alarmOn = false;
        sirenOn = false;

        if (alarmMSthread.joinable()) alarmMSthread.join();

        // Now it is safe to release the request and free the GPIO resources.
        cleanupAlarmMS();
      
        stopCameraSystem();

        cout << "Alarm OFF" << endl;
    }
    #endif
}

void toggleSiren() {
    // Only allow siren changes when alarm is armed
    if (!alarmOn) return;

    // Toggle siren state (main loop in main.cpp handles hardware toggling)
    sirenOn = !sirenOn;
    cout << "Siren " << (sirenOn ? "ON" : "OFF") << endl;
}

void alarmMSListener() {
    cout << "Alarm ON" << endl;
    
    // Block until an edge event is delivered by the kernel.
    // No polling loop is used here.
    while (alarmOn) {
        try {
#ifdef SIM
            if (simulateMS())
#else
            if (readAlarmMS())
#endif
            {
                if (!sirenOn) {
                    cout << "Motion detected! Triggering siren." << endl;
                    toggleSiren();
                }
            }
        } catch (const exception& e) {
            cerr << "Error reading motion sensor: " << e.what() << endl;
        }
    }

    cout << "Motion Sensor Listener stopped." << endl;
}

void lightsMSListener(bool firstEntry) {

    cout << "Lights motion sensor listener started." << endl;

    // Listener loop: same pattern as alarm, but always running.
    // No condition like alarmOn; just loop while the program is running.
    while (running) {
        try {
            if(lightsManualMode) return; // If manual mode is active, do not start the listener

        #ifdef SIM
            if (simulateMS()) {
                if (!lightsOn) {
                    simulateLED(true);
                    lightsOn = true;
                    cout << "LightsMS: motion detected, lights ON" << endl;
                }
                lightsOffToken.fetch_add(1);
            } else {
                if (lightsOn) {
                    // Simulate the falling edge / inactivity timer.
                    auto token = ++lightsOffToken;
                    thread([token]() {
                        this_thread::sleep_for(chrono::seconds(10));
                        //if (!running) return;
                        if (lightsManualMode) return; // Skip motion sensor control if lights are manually turned on
                        if (token == lightsOffToken.load()) {
                            simulateLED(false);
                        }
                    }).detach();
                    lightsOn = false;
                    cout << "LightsMS: motion ended, lights OFF scheduled" << endl;
                }
            }
            this_thread::sleep_for(chrono::seconds(2)); // SIM: poll every 2 sec
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
                // Falling edge: motion ended, start a 10s one-shot timer.
                if (lightsOn) {
                    auto token = ++lightsOffToken;
                    thread([token]() {
                        this_thread::sleep_for(chrono::seconds(3));
                        //if (!running) return;
                        if (lightsManualMode) return; 
                        if (token == lightsOffToken.load()) {
                            setLED(LIGHTS_LED, false);
                        }
                    }).detach();
                    lightsOn = false;
                    cout << "LightsMS: falling edge, lights OFF scheduled" << endl;
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

    cout << "Lights motion sensor listener stopped." << endl;
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

    #ifndef SIM
    // Make sure siren is turned off first, then disable the alarm
    if (sirenOn) toggleSiren();
    if (alarmOn) toggleAlarmActivation();

    cleanupLEDs();
    cleanupBuzzer(BUZZER_PIN);
    cleanupGate(GATE_PIN);
    dht11_api::cleanupDHT11();
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