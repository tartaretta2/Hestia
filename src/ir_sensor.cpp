#include "ir_sensor.h"
#include "houseControl.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

#ifndef SIM
    #include <gpiod.h>
#endif

using namespace std;

static IrCallback callback; // callback function to be called when a complete IR frame is received
static mutex g_mutex; // mutex to protect shared variables between the ISR thread and the rest of the code

#ifndef SIM

static gpiod_chip* g_chip = nullptr; // handle to the GPIO chip
static gpiod_line_request* g_request = nullptr; // contains the file descriptor on which the kernel notifies interrupts
static gpiod_edge_event_buffer* g_eventBuf = nullptr; // buffer containing the edge events read from the GPIO line
static thread g_irqThread; // thread that waits for interrupts
static atomic_bool g_threadRun = false;
static uint64_t g_lastTick = 0; // timestamp of the last edge in microseconds, used to calculate durations
static bool g_receiving = false; // true when we are currently receiving a frame 
static IrRawFrame g_buf[2]; // buffer for 2 frames: one is being filled while the other is being processed
static int g_active = 0; // current active buffer index receiving edges and building the frame

// The thread waits for edge events from the GPIO line and processes them to build IrRawFrame objects
static void irqThread()
{
    while (g_threadRun) {

        // wait for edge events with a timeout to exit the frame if no edges are received for a while
        int ret = gpiod_line_request_wait_edge_events(g_request, NEC_FRAME_TIMEOUT * 1000ULL); 

        if (ret < 0) {
            cerr << "[IR] wait_edge_events errore" << endl;
            break;
        }

        // Timeout: no edges received for a while, consider the frame complete
        if (ret == 0) {
            IrRawFrame frameToSend;
            bool hasFrame = false;
            {
                lock_guard<mutex> lock(g_mutex);
                if (g_receiving && g_buf[g_active].count > 0) {
                    frameToSend = g_buf[g_active]; // copy the frame to send 
                    hasFrame = true; 
                    g_active = 1 - g_active; 
                    g_buf[g_active] = IrRawFrame{}; // prepare the other buffer cell for the next frame
                }
                g_receiving = false;
            }
            if (hasFrame)
                callback(frameToSend); // call the callback outside the lock to avoid potential deadlocks
            continue;
        }

        // Edge detected, read and process all available events
        int n = gpiod_line_request_read_edge_events(g_request, g_eventBuf, 1);
        if (n < 0) continue;

        for (int i = 0; i < n; i++) {
            gpiod_edge_event* event = gpiod_edge_event_buffer_get_event(g_eventBuf, i);

            uint64_t tickUs = gpiod_edge_event_get_timestamp_ns(event) / 1000ULL; // convert nanoseconds to microseconds
            bool level = (gpiod_edge_event_get_event_type(event) == GPIOD_EDGE_EVENT_RISING_EDGE);

            lock_guard<mutex> lock(g_mutex);

            if (!g_receiving) {
                // update the last tick and check if we should start receiving a new frame
                uint64_t silence = tickUs - g_lastTick;
                g_lastTick = tickUs;

                // start receiving a new frame only when the line is low (falling edge) 
                // and the silence duration is greater than NEC_FRAME_TIMEOUT
                if (!level && silence >= NEC_FRAME_TIMEOUT) {
                    g_receiving = true;
                }
            } else {
                // already receiving a frame, record the edge duration and level
                IrRawFrame& frame = g_buf[g_active];
                if (frame.count < IR_MAX_EDGES) {
                    frame.edges[frame.count] = IrEdge(static_cast<uint32_t>(tickUs - g_lastTick), level);
                    frame.count++;
                }
                g_lastTick = tickUs; // update timestamp for next edge
            }
        }
    }
}

void initIR(IrCallback cb)
{
    callback = cb;

    g_chip = gpiod_chip_open(GPIO_CHIP);
    if (!g_chip) {
        cerr << "[IR] Error opening " << GPIO_CHIP << endl;
        return;
    }

    // GPIO Line settings for the IR sensor pin: input, pull-up, both edges
    gpiod_line_settings* settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);
    gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);

    gpiod_line_config* lineConfig = gpiod_line_config_new();
    unsigned int pin = IR_PIN;
    gpiod_line_config_add_line_settings(lineConfig, &pin, 1, settings);

    gpiod_request_config* reqConfig = gpiod_request_config_new();
    gpiod_request_config_set_consumer(reqConfig, "ir_sensor");

    // Request the line with the specified settings and get a request handle
    g_request = gpiod_chip_request_lines(g_chip, reqConfig, lineConfig);

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(lineConfig);
    gpiod_request_config_free(reqConfig);

    if (!g_request) {
        cerr << "[IR] gpiod_chip_request_lines fallito" << endl;
        gpiod_chip_close(g_chip);
        return;
    }

    g_eventBuf = gpiod_edge_event_buffer_new(1);
    g_threadRun = true;
    g_irqThread = thread(irqThread);

    cout << "[IR] Hardware ready on GPIO " << IR_PIN << endl;
}

void cleanupIR()
{
    g_threadRun = false;
    if (g_irqThread.joinable()) 
        g_irqThread.join();
    gpiod_edge_event_buffer_free(g_eventBuf);
    gpiod_line_request_release(g_request);
    gpiod_chip_close(g_chip);
    cout << "[IR] Hardware released" << endl;
}

#else

static thread g_simThread;
static atomic_bool g_simRun = false;

static IrRawFrame buildNecFrame(uint8_t cmd)
{
    IrRawFrame frame;
    uint8_t addr = 0x00;
    uint8_t addrInv = static_cast<uint8_t>(~addr);
    uint8_t cmdInv = static_cast<uint8_t>(~cmd);

    uint32_t data = (static_cast<uint32_t>(cmdInv) << 24) |
                    (static_cast<uint32_t>(cmd) << 16) |
                    (static_cast<uint32_t>(addrInv) << 8) |
                    (static_cast<uint32_t>(addr));

    int i = 0;
    frame.edges[i++] = IrEdge(NEC_LEADER_BURST, true);
    frame.edges[i++] = IrEdge(NEC_LEADER_SPACE, false);
    for (int bit = 0; bit < 32; bit++) {
        uint32_t space = (data & (1u << bit)) ? NEC_ONE_SPACE : NEC_ZERO_SPACE;
        frame.edges[i++] = IrEdge(NEC_BIT_BURST, true);
        frame.edges[i++] = IrEdge(space, false);
    }
    frame.edges[i++] = IrEdge(NEC_BIT_BURST, true);
    frame.count = i;
    return frame;
}

static const uint8_t SIM_KEYS[] = {
    //0x45,   POWER -> ShutdownSystem (avoided in simulation to prevent accidental shutdown)
    0x40,  // PLAY/PAUSE -> AlarmToggle
    0x47,  // FUNC/STOP -> ToggleLightsMode
    0x16,  // 0 -> ToggleHeatingMode
    0x0C,  // 1 -> LightsToggle
    0x18,  // 2 -> GateToggle
    0x5E,  // 3 -> ToggleACMode
    0x08,  // 4 -> ToggleAC
    0x1C   // 5 -> ToggleHeating
};

static void simThread()
{
    int idx = 0;
    while (g_simRun) {
        this_thread::sleep_for(chrono::seconds(5));
        if (!g_simRun) break;
        // simulate sending a key by building a frame and calling the callback
        IrRawFrame frame = buildNecFrame(SIM_KEYS[idx % sizeof(SIM_KEYS)]);
        idx++;
        callback(frame);
    }
}

void initIR(IrCallback cb)
{
    callback = cb;
    g_simRun = true;
    g_simThread = thread(simThread);
    cout << "[IR] Simulation on. A key every 5 seconds." << endl;
}

void cleanupIR()
{
    g_simRun = false;
    if (g_simThread.joinable()) g_simThread.join();
    cout << "[IR] Simulation stopped." << endl;
}

#endif