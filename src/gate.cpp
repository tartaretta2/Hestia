#include "gate.h"
#include "houseControl.h"
#include <iostream>
#include <thread>
#include <chrono>

#ifndef SIM
    #include <lgpio.h>
#endif

using namespace std;

#ifndef SIM
static int handle = -1;

// Pulse width (in microsecondi) inviato al servo SG90.
// 500us  -> 0   (cancello chiuso)
// 2500us -> 2500 -> 180 (cancello aperto)
static const int PULSE_CLOSED = 500;
static const int PULSE_OPEN = 2500;
static const int STEP_US = 100;      // ampiezza di ogni passo del movimento
static const int STEP_DELAY_MS = 15; // pausa tra un passo e l'altro (regola la velocit�)
static const int SERVO_FREQ_HZ = 50; // frequenza standard per servi hobby

// Stato del cancello, persistente tra le chiamate
static bool gateOpen = false;

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

    // Porta il servo in posizione nota (chiuso) all'avvio
    lgTxServo(handle, gatePin, PULSE_CLOSED, SERVO_FREQ_HZ, 0, 0);
    this_thread::sleep_for(chrono::milliseconds(400));
    lgTxServo(handle, gatePin, 0, SERVO_FREQ_HZ, 0, 0); // stop pulses: evita jitter/ronzio
    gateOpen = false;
}

// Muove il servo in modo graduale da 'from' a 'to' (pulse width in microsecondi).
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
        cout << "GATE: opening..." << endl;
        sweepServo(gatePin, PULSE_CLOSED, PULSE_OPEN);
        gateOpen = true;
        cout << "GATE: open" << endl;
    }
    else
    {
        cout << "GATE: closing..." << endl;
        sweepServo(gatePin, PULSE_OPEN, PULSE_CLOSED);
        gateOpen = false;
        cout << "GATE: closed" << endl;
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
    static bool open = false;
    if (!open)
    {
        cout << "GATE OPEN (simulated, 180 degrees)" << endl;
    }
    else
    {
        cout << "GATE CLOSED (simulated, 180 degrees)" << endl;
    }
    open = !open;
}