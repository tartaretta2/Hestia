#include "motion_sensor.h"
#include "led.h"
#include "buzzer.h"
#include "ir_sensor.h"
#include "ir_decoder.h"
#include "ir_remote.h"
#include "gate.h"
#include "houseControl.h"
#include "cameraYOLO.h"
#include <iostream>
#include <thread>
#include <atomic>

using namespace std;

// Global state flags shared across threads
// (atomic ensures safe concurrent access)
atomic<bool> sirenOn(false);   // true when the buzzer/LED should be active
atomic<bool> alarmOn(false);   // true when the alarm system is armed
atomic<bool> running(true);    // main loop continues while true

// Called by the IR receiver when a complete NEC frame is captured.
// It decodes the frame and dispatches the resulting command.
void onRawFrame(const IrRawFrame& raw)
{
    NecFrame frame = decodeNEC(raw);

    if (!frame.valid) return;

    if (frame.isRepeat) {
        //cout << "[IR] <REPEAT> (Same key as before)" << endl;
        return;
    }

    handleKey(frame.command);
}

int main() {
#ifndef SIM
    initBuzzer(GPIO_CHIP, BUZZER_PIN);
    initAlarmLED(GPIO_CHIP, ALARM_LED);
    initLightsLED(GPIO_CHIP, LIGHTS_LED);
    initLightsMS(GPIO_CHIP, LIGHTS_MS);
    initIR(onRawFrame);
    initGate(GPIO_CHIP, GATE_PIN);
    startLightsListener();
    #endif

    cout << "Press remote POWER button to turn on the alarm system." << endl;

    cout << "Press remote PLAY/PAUSE button to turn on the alarm system." << endl;

    cout << "Press remote POWER button to turn off the whole system." << endl;

    while (running) {

        if (sirenOn) {
            #ifdef SIM
                simulateLED();
                simulateBuzzer();
            #else
                toggleAlarmLED(ALARM_LED);
                toggleBuzzer(BUZZER_PIN);
            #endif
        } else {
            // Avoid busy waiting when siren is off
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }

    // Ensure system is fully stopped before exiting
    // (stops siren/alarm/lights threads and releases GPIO resources)
    shutdownSystem();

    cout << "System shut down." << endl;
    return 0;
}