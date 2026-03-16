#include "ir_sensor.h"
#include "ir_decoder.h"
#include "ir_remote.h"
#include <iostream>
#include <thread>
#include <atomic>

using namespace std;

atomic<bool> running(true);

// Called every time a raw frame get caught by the sensor, it decodes the NEC frame and manage the key pressed
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
{    cout << "Smart House IR Remote" << endl;
#ifdef SIM
    cout << "Simulation mode, hardware independent" << endl;
#else
    cout << "On-board mode, code is running on HW on a Raspberry Pi 5. IR sensor on port GPIO " << IR_PIN << "" << endl;
#endif
    cout << "Press enter to escape." << endl;

    initIR(onRawFrame); 

    string line;
    getline(cin, line);

    running = false;
    cleanupIR();
    cout << "Shut down." << endl;
    return 0;
}
