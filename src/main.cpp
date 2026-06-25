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
atomic<bool> lightsOn(false);  // true when the lights are on
atomic<bool> gateOpen(false);   // true when the gate is open
atomic<bool> lightsOnManually(false); // true when the lights are manually turned on (not by motion sensor)

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
    if (server_fd < 0) return;

    sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY); 
    address.sin_port = htons(12345);      

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    ::bind(server_fd, (struct sockaddr*)&address, sizeof(address));

    // Set a timeout for recvfrom to avoid blocking indefinitely
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    char buffer[1024];
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    cout << "[System] Web command thread ready on port 12345..." << std::endl;

    while (running) { 
        memset(buffer, 0, sizeof(buffer));
        
        ssize_t bytes_received = recvfrom(server_fd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&client_addr, &client_len); 
        
        if (bytes_received <= 0) {
            continue; 
        }

        string command(buffer);

        if (command == "getState") {
            string stateReply = "ALARM:" + to_string(alarmOn ? 1 : 0) + 
                "|LIGHTS:" + to_string(lightsOn ? 1 : 0) +
                "|GATE:" + to_string(gateOpen ? 1 : 0); 
            
            // Respond with the current state of the system (alarm and lights)
            sendto(server_fd, stateReply.c_str(), stateReply.length(), 0, (struct sockaddr*)&client_addr, client_len);
            continue; 
        }

        cout << "[System] Received web command: [" << command << "]" << endl;

        if (command == "toggleAlarm") {
            cout << "[Alarm] Alarm toggled via Web." << endl;
            toggleAlarmActivation(); 
        } 
        else if (command == "toggleLights") {
            cout << "[Lights] Lights toggled via Web." << endl;
            toggleLightsActivation();
        } 
        else if (command == "openGate") {
            cout << "[Gate] Gate toggled via Web." << endl;
            toggleGateActivation();
        }
        else if (command == "shutdownSystem") {
            cout << "[System] Shutdown requested via Web." << endl;
            running = false; // Signal the main loop to exit
        }
        else {
            cout << "[System] Unknown command received: [" << command << "]" << endl;
        }
        // Acknowledge the received command to the web server
        string ack = "OK";
        sendto(server_fd, ack.c_str(), ack.length(), 0, (struct sockaddr*)&client_addr, client_len);
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
            cout << "[DHT11] Temp: " << temp << " C  |  Hum: " << hum << " %" << endl;
            bool shouldBeOn = (temp > AC_THRESHOLD_C);
            if (shouldBeOn && !acOn) {
                // transition from cool to hot (turn the AC on)
                cout << "[AC] turned ON (hot detected: " << temp << " C - " << hum << " %)" << endl;
                #ifndef SIM
                    setLED(TEMP_LED, true);
                #else
                    simulateLED(true);
                #endif
            } else if (!shouldBeOn && acOn) {
                // transition from hot to cool (turn the AC off)
                cout << "[AC] turned OFF (cool again: " << temp << " C - " << hum << " %)" << endl;
                #ifndef SIM
                    setLED(TEMP_LED, false);
                #else
                    simulateLED(false);
                #endif
            }
            acOn = shouldBeOn;
        } else {
            cout << "[DHT11] Read failed." << endl;
        }
        this_thread::sleep_for(chrono::seconds(3));
    }
}

extern atomic<bool> disarmRequested;

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

    if (tempThread.joinable()) {
        tempThread.join();
    }

    cout << "Shutting down web interface..." << endl;    
    if (webThread.joinable()) {
        webThread.join(); 
    }

#ifndef SIM
    // Ensure system is fully stopped before exiting
    // (stops siren/alarm/lights threads and releases GPIO resources)
    shutdownSystem();
#endif
    cout << "System shut down." << endl;
    return 0;
}