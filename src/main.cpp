#include "motion_sensor.h"
#include "led.h"
#include "buzzer.h"
#include "ir_sensor.h"
#include "ir_decoder.h"
#include "ir_remote.h"
#include "gate.h"
#include "houseControl.h"
#include "cameraYOLO.h"
#include "Dht11.h"
#include <iostream>
#include <thread>
#include <atomic>

using namespace std;
using namespace dht11_api;

// Global state flags shared across threads
// (atomic ensures safe concurrent access)
atomic<bool> sirenOn(false);   // true when the buzzer/LED should be active
atomic<bool> alarmOn(false);   // true when the alarm system is armed
atomic<bool> running(true);    // main loop continues while true

// Heater threshold: turn the heater LED on when temperature drops below it
constexpr float HEATER_THRESHOLD_C =20.0f;

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

// Reads DHT11 periodically and triggers the heater LED when below threshold.
void temperatureMonitor()
{
    bool heaterOn = false;
    while (running) {
        float temp = 0.0f, hum = 0.0f;
        if (readDHT11(temp, hum)) {
            cout << "[DHT11] Temp: " << temp << " C  |  Humidity: " << hum << " %" << endl;
            bool shouldBeOn = (temp < HEATER_THRESHOLD_C);
            if (shouldBeOn && !heaterOn) {
                // Transition warm -> cold: signal the heater is needed
                cout << "[HEATER] ON (cold detected)" << endl;
                #ifndef SIM
                    setLED(TEMP_LED, true);
                #else
                    simulateLED();
                #endif
            } else if (!shouldBeOn && heaterOn) {
                cout << "[HEATER] OFF (warm again)" << endl;
            }
            heaterOn = shouldBeOn;
        } else {
            cout << "[DHT11] Read failed." << endl;
        }
        this_thread::sleep_for(chrono::seconds(3));
    }
}

int main() {
#ifndef SIM
    initBuzzer(GPIO_CHIP, BUZZER_PIN);
    initAlarmLED(GPIO_CHIP, ALARM_LED);
    initLightsLED(GPIO_CHIP, LIGHTS_LED);
    initLightsMS(GPIO_CHIP, LIGHTS_MS);
    initTempLED(GPIO_CHIP, TEMP_LED);
    initIR(onRawFrame);
    initDHT11();   
    initGate(GPIO_CHIP, GATE_PIN);
    startLightsListener();
    #endif

    thread tempThread(temperatureMonitor);

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

    tempThread.join();

#ifndef SIM
    // Ensure system is fully stopped before exiting
    // (stops siren/alarm/lights threads and releases GPIO resources)
    shutdownSystem();
#endif
    cout << "System shut down." << endl;
    return 0;
}