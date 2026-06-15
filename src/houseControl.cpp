#include "houseControl.h"
#include "motion_sensor.h"
#include "led.h"
#include "buzzer.h"
#include "ir_remote.h"
#include <iostream>
#include <thread>
#include <atomic>
using namespace std;


// Shared state variables defined in main.cpp
extern atomic<bool> sirenOn;
extern atomic<bool> alarmOn;
extern atomic<bool> running;


// Threads used for asynchronous tasks
static thread motionThread; // thread that monitors the motion sensor when the alarm is armed
static thread sirenThread;  // reserved for future use (e.g., separate siren task)


void toggleAlarmActivation() {
   if (!alarmOn) {
       // Arm the alarm system: motion sensor starts monitoring
       alarmOn = true;
       cout << "Alarm ON" << endl;
       motionThread = thread(MSListener);


   } else {
       // Disarm the alarm system: stop siren and stop motion monitoring
       alarmOn = false;
       sirenOn = false;
       cout << "Alarm OFF" << endl;


       if (motionThread.joinable()) motionThread.join();
   }
}


void toggleSiren() {
   // Only allow siren control when alarm is armed
   if (!alarmOn) return;


   // Toggle siren state (main loop handles hardware based on this variable)
   sirenOn = !sirenOn;
   cout << "Siren " << (sirenOn ? "ON" : "OFF") << endl;
}


void MSListener() {
   cout << "Motion Sensor Listener started." << endl;


   // Check motion sensor state in a loop while the alarm is armed.
   // If motion is detected, the siren is enabled.
   while (alarmOn) {
       try {
           #ifdef SIM
           if (simulateMS())
           #else
           if (readMS())
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


void shutdownSystem(){
   // Quit main loop and ensure all threads are stopped before exiting the program
   running = false;


   // Make sure siren is turned off first, then disable the alarm
   if (sirenOn) toggleSiren();
   if (alarmOn) toggleAlarmActivation();
}




