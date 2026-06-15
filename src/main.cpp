#include "motion_sensor.h"
#include "led.h"
#include "buzzer.h"
#include "ir_sensor.h"
#include "ir_decoder.h"
#include "ir_remote.h"
#include <iostream>
#include <thread>
#include <atomic>

#ifndef SIM
    #define GPIO_CHIP "/dev/gpiochip4"
    #define MS_PIN 17
    #define BUZZER_PIN 27
    #define LED_PIN 22
#endif

using namespace std;

atomic<bool> running(true);
atomic<bool> alarmOn(false);
atomic<bool> sirenOn(false);

int readSensor()
{
    #ifdef SIM
      return simulateMS();
    #else
        return readMS();
    #endif
}

void alarm()
{
    int i;
    while (sirenOn)
    {
        #ifndef SIM
            toggleLED(LED_PIN);
            toggleBuzzer(BUZZER_PIN);
            this_thread::sleep_for(chrono::milliseconds(500));
        #else
            simulateLED();
            simulateBuzzer();
        #endif
    }
}

void MSListener()
{

    cout << "Motion Sensor Listener started." << endl;
    while (alarmOn)
    {
        if (readSensor())
        {
            cout << "Motion detected!" << endl;
            sirenOn = true;
            thread siren(alarm);
            cout << "Alarm triggered. Press enter to stop." << endl;
            cin.get();
            sirenOn = false;
            siren.join();
        }
    }
}

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

int main()
{
#ifndef SIM
    initBuzzer(GPIO_CHIP, BUZZER_PIN);
    initLED(GPIO_CHIP, LED_PIN);
    initMS(GPIO_CHIP, MS_PIN);
    initIR(onRawFrame); 
#endif

    while (running)
    {
        cout << "Press Enter to turn on the alarm system." << endl;
        cin.get();
        alarmOn = true;
        thread listener(MSListener);
        cout << "Alarm activated. Press Enter to stop." << endl;
        cin.get();
        alarmOn = false;
        listener.join();
    }

#ifndef SIM
    cleanupBuzzer(BUZZER_PIN);
    cleanupLED();
    cleanupIR();
#endif

    cout << "System shut down." << endl;

    return 0;
}
