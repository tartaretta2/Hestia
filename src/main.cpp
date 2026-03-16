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
    cerr << "Debug: main partito" << endl;

    cout << "=== Smart House IR Remote ===" << endl;
#ifdef SIM
    cout << "Modalita': SIMULAZIONE" << endl;
#else
    cout << "Modalita': HARDWARE (GPIO " << IR_PIN << ")" << endl;
#endif
    cout << "Premi Invio per uscire." << endl;

    initIR(onRawFrame);  // avvia dopo i cout, cos� li vedi sempre

    string line;
    getline(cin, line);  // aspetta Invio, senza consumare nulla prima

    running = false;
    cleanupIR();
    cout << "Sistema spento." << endl;
    return 0;
}
