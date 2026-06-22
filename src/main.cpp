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
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring> // Added for std::memset

using namespace std;
using namespace dht11_api;

// Global state flags shared across threads
// (atomic ensures safe concurrent access)
atomic<bool> sirenOn(false);   // true when the buzzer/LED should be active
atomic<bool> alarmOn(false);   // true when the alarm system is armed
atomic<bool> running(true);    // main loop continues while true

// AC threshold: turn the AC led on when temperature > 26C
constexpr float AC_THRESHOLD_C = 26.0f;

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

// This function runs in a detached thread and listens for UDP commands from the Flask web server
void webCommandHandler() {
    int server_fd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (server_fd < 0) {
        std::cerr << "[C++] Error creating socket!" << std::endl;
        return;
    }

    sockaddr_in address;
    // Wipe memory to prevent garbage data
    std::memset(&address, 0, sizeof(address));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY); // Bind to all interfaces
    address.sin_port = htons(12345);      

    // Enable address reuse to avoid "Address already in use" errors on quick restarts
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (::bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "[C++] Socket BIND error on port 12345! Error code: " << errno << std::endl;
        close(server_fd);
        return;
    }
    
    char buffer[1024];
    cout << "[C++] Web command thread started successfully on port 12345..." << std::endl;

    while (running) { // Changed to respect the global 'running' flag
        std::memset(buffer, 0, sizeof(buffer));
        
        // Receive the command
        ssize_t bytes_received = recvfrom(server_fd, buffer, sizeof(buffer) - 1, 0, nullptr, nullptr); 
        if (bytes_received <= 0) continue;

        std::string command(buffer);
        std::cout << "[C++] Received web command: [" << command << "]" << std::endl;

        // Match the strings sent by the Python backend
        if (command == "toggleAlarm") {
            // Using your existing toggle function from houseControl.h
            toggleAlarmActivation(); 
            std::cout << "[C++] Alarm toggled via Web." << std::endl;
        } 
        else if (command == "toggleLights") {
            // NOTE: Replace this with your actual lights toggle function!
            // setLED(LIGHTS_LED, !lightsOn);
            std::cout << "[C++] Lights toggled via Web." << std::endl;
        } 
        else if (command == "openGate") {
            // NOTE: Replace this with your actual gate function from gate.h!
            toggleGateActivation();
            std::cout << "[C++] Gate opened via Web." << std::endl;
        } 
        else {
            std::cout << "[C++] Unknown command ignored: " << command << std::endl;
        }
    }
    close(server_fd);
}


// Reads DHT11 periodically and triggers the AC LED when above threshold
void temperatureMonitor()
{
    bool acOn = false;
    while (running) {
        float temp = 0.0f, hum = 0.0f;
        if (readDHT11(temp, hum)) {
            // cout << "[DHT11] Temp: " << temp << " C  |  Humidity: " << hum << " %" << endl;
            bool shouldBeOn = (temp > AC_THRESHOLD_C);
            if (shouldBeOn && !acOn) {
                // transition from cool to hot (turn the AC on)
                cout << "[AC] ON (hot detected)" << endl;
                #ifndef SIM
                    setLED(TEMP_LED, true);
                #else
                    simulateLED(true);
                #endif
            } else if (!shouldBeOn && acOn) {
                // transition from hot to cool (turn the AC off)
                cout << "[AC] OFF (cool again)" << endl;
                #ifndef SIM
                    setLED(TEMP_LED, false);
                #else
                    simulateLED(false);
                #endif
            }
            acOn = shouldBeOn;
        } else {
            // cout << "[DHT11] Read failed." << endl;
        }
        this_thread::sleep_for(chrono::seconds(3));
    }
}

extern std::atomic<bool> disarmRequested;

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

    // Start the web commands handler thread
    thread webThread(webCommandHandler);
    webThread.detach(); // Detach allows it to run independently in the background

    cout << "Press remote PLAY/PAUSE button to turn on the alarm system." << endl;
    cout << "Press remote POWER button to turn off the whole system." << endl;

    while (running) {
        
        if (disarmRequested.exchange(false)) {
            if (alarmOn) toggleAlarmActivation();
        }

        if (sirenOn) {
            #ifdef SIM
                simulateLED(true);
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