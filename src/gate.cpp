#include "gate.h"
#include "houseControl.h"
#include <iostream>
#include <thread>
#include <chrono>

#ifndef SIM
    #include <lgpio.h>
#endif

using namespace std;

extern atomic<bool> gateOpen;

#ifndef SIM
static int handle = -1;

// Pulse width (in microseconds) sent to the SG90 servo
// 500us  -> 0°   (gate closed)
// 2500us -> 180° (gate open)
static const int PULSE_CLOSED = 500;
static const int PULSE_OPEN = 2500;
static const int STEP_US = 100;      // step in microseconds for gradual movement of the servo
static const int STEP_DELAY_MS = 15; // pause between steps (regulates the speed)
static const int SERVO_FREQ_HZ = 50; // standard frequency for servos

void initGate(const char *gpioChip, const unsigned int gatePin)
{
    handle = lgGpiochipOpen(4);
    cout << "Initializing GATE on pin: " << gatePin << endl;
    if (handle < 0)
    {
        cerr << "Failed to open GPIO chip" << endl;
        return;
    }
    lgGpioClaimOutput(handle, 0, gatePin, 0);

    // Reset servo to closed position
    lgTxServo(handle, gatePin, PULSE_CLOSED, SERVO_FREQ_HZ, 0, 0);
    this_thread::sleep_for(chrono::milliseconds(400));
    lgTxServo(handle, gatePin, 0, SERVO_FREQ_HZ, 0, 0); // stop pulses: avoid jitter and noise
    gateOpen = false;
}

// Gradually move the servo
static void sweepServo(const unsigned int gatePin, int from, int to)
{
    if (from < to)
    {
        for (int p = from; p <= to; p += STEP_US)
        {
            lgTxServo(handle, gatePin, p, SERVO_FREQ_HZ, 0, 0);
            this_thread::sleep_for(chrono::milliseconds(STEP_DELAY_MS));
        }
    }
    else
    {
        for (int p = from; p >= to; p -= STEP_US)
        {
            lgTxServo(handle, gatePin, p, SERVO_FREQ_HZ, 0, 0);
            this_thread::sleep_for(chrono::milliseconds(STEP_DELAY_MS));
        }
    }

    lgTxServo(handle, gatePin, to, SERVO_FREQ_HZ, 0, 0);
    this_thread::sleep_for(chrono::milliseconds(50));
    lgTxServo(handle, gatePin, 0, SERVO_FREQ_HZ, 0, 0);
}

void toggleGate(const unsigned int gatePin)
{
    if (!gateOpen)
    {
        cout << "Gate: opening..." << endl;
        sweepServo(gatePin, PULSE_CLOSED, PULSE_OPEN);
        gateOpen = true;
    }
    else
    {
        cout << "Gate: closing..." << endl;
        sweepServo(gatePin, PULSE_OPEN, PULSE_CLOSED);
        gateOpen = false;
    }
}

void cleanupGate(const unsigned int gatePin)
{
    lgTxServo(handle, gatePin, 0, SERVO_FREQ_HZ, 0, 0);
    lgGpiochipClose(handle);
}

#endif

void simulateGate()
{
    if (!gateOpen)
    {
        cout << "Gate opened (simulated)" << endl;
    }
    else
    {
        cout << "Gate closed (simulated)" << endl;
    }
    gateOpen = !gateOpen;
}