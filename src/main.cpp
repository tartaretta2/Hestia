#include "ir_sensor.h"
#include "ir_decoder.h"
#include "ir_remote.h"
#include <iostream>
#include <thread>
#include <atomic>

using namespace std;

atomic<bool> running(true);

// Chiamata ogni volta che arriva un frame grezzo dal sensore.
// Decodifica il frame NEC e gestisce il tasto premuto.
void onRawFrame(const IrRawFrame& raw)
{
    NecFrame frame = decodeNEC(raw);

    if (!frame.valid) return;

    if (frame.isRepeat) {
        cout << "[IR] <REPEAT>" << endl;
        return;
    }

    handleKey(frame.command);
}

int main()
{
#ifndef SIM
    initIR(onRawFrame);
#else
    initIR(onRawFrame);
#endif

    cout << "=== Smart House IR Remote ===" << endl;
#ifdef SIM
    cout << "Modalita': SIMULAZIONE" << endl;
#else
    cout << "Modalita': HARDWARE (GPIO " << IR_PIN << ")" << endl;
#endif
    cout << "Premi Invio per uscire." << endl;
    cin.ignore();
    cin.get();
    running = false;

    cleanupIR();
    cout << "Sistema spento." << endl;
    return 0;
}
