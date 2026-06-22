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

//targa autorizzate
static const vector<string> authorizedPlates = {
    "CZ889KF"
};


// Shared state variables defined in main.cpp
extern atomic<bool> sirenOn;
extern atomic<bool> alarmOn;
extern atomic<bool> running;


// Threads used for asynchronous tasks
static thread alarmMSthread;   // thread that blocks on alarm motion-sensor edge events
static thread lightsThread;    // thread that blocks on lights motion-sensor edge events
static atomic<uint64_t> lightsOffToken(0); // token used to check if thread should turn off lights
atomic<bool> disarmRequested(false); //flag set by plate recognition to request disarming the alarm system (checked in main loop)

void toggleAlarmActivation() {
    #ifdef SIM
        alarmOn = !alarmOn;
        cout << (alarmOn ? "Alarm OFF" : "Alarm ON") << endl;
    #else
    if (!alarmOn) {
        // Arm the alarm system: create the GPIO request and start the listener.
        if (!initAlarmMS(GPIO_CHIP, ALARM_MS)) {
            cerr << "Unable to arm alarm: motion sensor init failed." << endl;
            return;
        }

        alarmOn = true;
        cout << "Alarm ON" << endl;

        // If the PIR is already active when arming, trigger immediately so
        // we do not miss the first state after a disarm/re-arm cycle.
        if (isAlarmMSActive() && !sirenOn) {
            cout << "Motion already active, triggering siren immediately." << endl;
            toggleSiren();
        }

        // Start the thread that blocks until a motion edge arrives.
        alarmMSthread = thread(alarmMSListener);
        
        //Start the camera for plate recognition
        initCameraSystem();

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
    cout << "Alarm motion sensor listener started." << endl;

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

void lightsMSListener() {
    cout << "Lights motion sensor listener started." << endl;

    bool lightsOn = false;

    // Listener loop: same pattern as alarm, but always running.
    // No condition like alarmOn; just loop while the program is running.
    while (running) {
        try {
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
                        if (!running) return;
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
                        if (!running) return;
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

    // Wait for all listener threads to cleanly exit
    if (lightsThread.joinable()) lightsThread.join();
    cout << "Lights motion sensor listener stopped." << endl;
}

void startLightsListener() {
    if (lightsThread.joinable()) return; // already running
    lightsThread = thread(lightsMSListener);
}

void toggleGateActivation() {
    #ifdef SIM
        simulateGate();
    #else
        toggleGate(GATE_PIN);
    #endif
}

void shutdownSystem(){
    // Request main loop and all listeners to exit
    running = false;

    #ifndef SIM
    // Make sure siren is turned off first, then disable the alarm
    setLED(ALARM_LED, false);
    setLED(LIGHTS_LED, false);
    if (sirenOn) toggleSiren();
    if (alarmOn) toggleAlarmActivation();

    cleanupIR();
    cleanupBuzzer(BUZZER_PIN);
    cleanupLEDs();
    cleanupAlarmMS();
    cleanupLightsMS();
    cleanupGate(GATE_PIN);
    dht11_api::cleanupDHT11();
    #endif
}


void checkPlate(const string& plate) {
    for (const auto& p : authorizedPlates) {
        if (plate == p) {
            cout << "Targa autorizzata riconosciuta: " << plate << endl;

            if (alarmOn) {
                cout << "Disarmo richiesto da targa riconosciuta" << endl;
                disarmRequested = true;
            }
            return;
        }
    }
    cout << "Targa non autorizzata: " << plate << endl;
}



