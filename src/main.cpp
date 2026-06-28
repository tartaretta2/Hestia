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

// Global state flags shared across threads
// atomic ensures safe concurrent access
atomic<bool> sirenOn(false);   
atomic<bool> alarmOn(false);   
atomic<bool> running(true);    // main loop continues while true
atomic<bool> lightsOn(false); 
atomic<bool> gateOpen(false);
atomic<bool> armRequested(false); // true when the alarm system should be armed (set by web interface)
atomic<bool> disarmRequested(false); // true when the alarm system should be disarmed (set by camera or web interface)
atomic<bool> lightsManualMode(true);
atomic<bool> acManualMode(false); 
atomic<bool> heatingManualMode(false); 
atomic<bool> acOn(false); 
atomic<bool> heatingOn(false); 


// Called by the IR receiver when a complete NEC frame is captured
// It decodes the frame and dispatches the resulting command
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
    std::memset(&address, 0, sizeof(address)); // Clean the structure to avoid garbage values
    address.sin_family = AF_INET; // Sets the address family to IPv4
    address.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any IP address
    address.sin_port = htons(12345); // Listen on port 12345 for incoming UDP packets

    // Set a timeout for recvfrom to avoid blocking indefinitely
    // and allow address reuse to avoid "Address already in use" errors on restart
    int opt = 1; // 1 true, 0 false
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    ::bind(server_fd, (struct sockaddr*)&address, sizeof(address)); // Bind the socket to the specified address and port
    
    char buffer[1024];
    sockaddr_in client_addr; // address from which the command was received
    socklen_t client_len = sizeof(client_addr);

    cout << "[System] Web command thread ready on port 12345..." << endl;

    while (running) { 
        memset(buffer, 0, sizeof(buffer));
        
        ssize_t bytes_received = recvfrom(server_fd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&client_addr, &client_len); 
        
        if (bytes_received <= 0) {
            continue; 
        }

        string command(buffer);

        if (command == "getState") {
            // Respond with the current state of the system (alarm and lights)
            string stateReply = "ALARM:" + to_string(alarmOn ? 1 : 0) + 
                "|LMODE:" + to_string(lightsManualMode ? 1 : 0) +
                "|LIGHTS:" + to_string(lightsOn ? 1 : 0) +
                "|GATE:" + to_string(gateOpen ? 1 : 0) +
                "|ACMODE:" + to_string(acManualMode ? 1 : 0) + 
                "|AC:" + to_string(acOn ? 1 : 0) +
                "|HEATINGMODE:" + to_string(heatingManualMode ? 1 : 0) + 
                "|HEATING:" + to_string(heatingOn ? 1 : 0);

            sendto(server_fd, stateReply.c_str(), stateReply.length(), 0, (struct sockaddr*)&client_addr, client_len);
            continue; 
        }

        cout << "[System] Received web command: [" << command << "]" << endl;

        if(command == "toggleAlarm") {
            cout << "[Alarm] Alarm toggled via Web." << endl;
            if (!alarmOn) {
                armRequested = true; // Request to arm the alarm system
            } else {
                disarmRequested = true; // Request to disarm the alarm system
            }
        } else if(command == "toggleLights") {
            cout << "[Lights] Lights toggled via Web." << endl;
            toggleLightsActivation();
        } else if(command == "toggleGate") {
            cout << "[Gate] Gate toggled via Web." << endl;
            toggleGateActivation();
        } else if(command == "shutdownSystem") {
            cout << "[System] Shutdown requested via Web." << endl;
            running = false; // Request main loop to exit
        } else if (command =="toggleLightsMode") {
            cout << "[Lights] Lights mode toggled via Web." << endl; 
            toggleLightsMode();
        } else if (command =="toggleACMode") {
            cout << "[AC] AC mode toggled via Web." << endl; 
            toggleACMode();
        } else if (command =="toggleAC") {
            cout << "[AC] AC toggled via Web." << endl;
            toggleAC();
        } else if (command =="toggleHeatingMode") {
            cout << "[HEATING] Heating mode toggled via Web." << endl; 
            toggleHeatingMode();
        } else if (command =="toggleHeating") {
            cout << "[HEATING] Heating toggled via Web." << endl;
            toggleHeating();
        } else {
            cout << "[System] Unknown command received: [" << command << "]" << endl;
        }

        // Acknowledge the received command to the web server
        string ack = "OK";
        sendto(server_fd, ack.c_str(), ack.length(), 0, (struct sockaddr*)&client_addr, client_len);
    }
    close(server_fd);
}

int main() {
#ifndef SIM
    initBuzzer(GPIO_CHIP, BUZZER_PIN);
    initLEDs(GPIO_CHIP, ALARM_LED, LIGHTS_LED, AC_LED, HEATING_LED);
    initDHT11();   
    initGate(GPIO_CHIP, GATE_PIN);
#endif

    initIR(onRawFrame);
    startTemperatureMonitor();

    #ifndef SIM
    cout << "System initialized. Waiting for commands..." << endl;
    cout << "Press remote:" << endl 
        << " POWER button to turn off the whole system" << endl
        << " PLAY/PAUSE button to turn on the alarm system" << endl
        << " FUNC/STOP button to toggle lights mode (manual/automatic)" << endl
        << " 0 button to toggle heating mode (manual/automatic)" << endl
        << " 1 to toggle lights" << endl
        << " 2 to toggle gate" << endl
        << " 3 button to toggle AC mode (manual/automatic)" << endl
        << " 4 to toggle AC" << endl
        << " 5 to toggle heating" << endl;
    #endif

    // Start the web commands handler thread
    thread webThread(webCommandHandler);

    while (running) {
        
        if (armRequested.exchange(false)) {
            cout << "[Alarm] Alarm system arming requested" << endl;
            toggleAlarmActivation();
        }

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

    cout << "Shutting down web interface..." << endl;    
    if (webThread.joinable()) {
        webThread.join(); 
    }

#ifndef SIM
    // Ensure system is fully stopped before exiting
    // stops siren/alarm/lights threads and releases GPIO resources
    shutdownSystem();
#endif
    cout << "System shut down." << endl;
    return 0;
}