#include "ir_sensor.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

#ifndef SIM
    #include <gpiod.h>
#endif

using namespace std;

static IrCallback g_callback;
static mutex g_mutex;

// On-board mode - HW dependent
#ifndef SIM

static gpiod_chip* g_chip = nullptr;
static gpiod_line_request* g_request = nullptr;
static gpiod_edge_event_buffer* g_eventBuf = nullptr;
static thread g_irqThread;
static atomic<bool> g_threadRun = false;
static uint64_t g_lastTick = 0;
static bool g_receiving = false;
static IrRawFrame g_buf[2];
static int g_active = 0;

// Thread interrupt-driven: gpiod_line_request_wait_edge_events blocca
// il thread finchè il kernel non riceve un interrupt hardware dal GPIO.
// Non consuma CPU mentre aspetta.
static void irqThread()
{
    while (g_threadRun) {

        int ret = gpiod_line_request_wait_edge_events(
                      g_request, NEC_FRAME_TIMEOUT * 1000ULL);

        if (ret < 0) {
            cerr << "[IR] wait_edge_events errore" << endl;
            break;
        }

        if (ret == 0) {
            // Timeout: silenzio > 15ms -> fine frame
            IrRawFrame frameToSend;
            bool hasFrame = false;
            {
                lock_guard<mutex> lock(g_mutex);
                if (g_receiving && g_buf[g_active].count > 0) {
                    frameToSend = g_buf[g_active];
                    hasFrame    = true;
                    g_active    = 1 - g_active;
                    g_buf[g_active] = IrRawFrame{};
                }
                g_receiving = false;
            }
            if (hasFrame)
                g_callback(frameToSend);
            continue;
        }

        // Edge arrivato: leggi l'evento
        int n = gpiod_line_request_read_edge_events(g_request, g_eventBuf, 1);
        if (n < 0) continue;

        for (int i = 0; i < n; i++) {
            gpiod_edge_event* event =
                gpiod_edge_event_buffer_get_event(g_eventBuf, i);

            uint64_t tickUs = gpiod_edge_event_get_timestamp_ns(event) / 1000ULL;
            bool level = (gpiod_edge_event_get_event_type(event)
                          == GPIOD_EDGE_EVENT_RISING_EDGE);

            lock_guard<mutex> lock(g_mutex);

            if (!g_receiving) {
                // Aggiorna sempre lastTick per misurare il silenzio
                uint64_t silenzio = tickUs - g_lastTick;
                g_lastTick = tickUs;

                // Inizia a registrare solo su fronte di DISCESA (inizio burst)
                // e solo dopo un silenzio lungo almeno NEC_FRAME_TIMEOUT:
                // questo garantisce che siamo all'inizio del frame
                if (!level && silenzio >= NEC_FRAME_TIMEOUT) {
                    g_receiving = true;
                }
            } else {
                // Siamo dentro un frame: registra durata e livello
                IrRawFrame& frame = g_buf[g_active];
                if (frame.count < IR_MAX_EDGES) {
                    frame.edges[frame.count] = IrEdge(
                        static_cast<uint32_t>(tickUs - g_lastTick), level);
                    frame.count++;
                }
                g_lastTick = tickUs;
            }
        }
    }
}

void initIR(IrCallback cb)
{
    g_callback = cb;

    g_chip = gpiod_chip_open("/dev/gpiochip4");
    if (!g_chip) {
        cerr << "[IR] Error opening /dev/gpiochip4" << endl;
        return;
    }

    // Crea la configurazione della linea
    gpiod_line_settings* settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);
    gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);

    gpiod_line_config* lineConfig = gpiod_line_config_new();
    unsigned int pin = IR_PIN;
    gpiod_line_config_add_line_settings(lineConfig, &pin, 1, settings);

    gpiod_request_config* reqConfig = gpiod_request_config_new();
    gpiod_request_config_set_consumer(reqConfig, "ir_sensor");

    // Richiedi la linea con interrupt su entrambi i fronti
    g_request = gpiod_chip_request_lines(g_chip, reqConfig, lineConfig);

    // Libera le configurazioni temporanee
    gpiod_line_settings_free(settings);
    gpiod_line_config_free(lineConfig);
    gpiod_request_config_free(reqConfig);

    if (!g_request) {
        cerr << "[IR] gpiod_chip_request_lines fallito" << endl;
        gpiod_chip_close(g_chip);
        return;
    }

    // Buffer per leggere gli eventi
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
    cout << "[IR] Hardware released." << endl;
}

// Simulation mode - HW independent
#else

static thread g_simThread;
static atomic<bool> g_simRun = false;

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
    0x45,  // POWER  -> AlarmToggle
    0x0C,  // 1      -> LightsToggle
    0x18,  // 2      -> GateToggle
    0x5E,  // 3      -> HeatingToggle
    0x46,  // VOL+   -> LightsUp
    0x15,  // VOL-   -> LightsDown
};

static void simThread()
{
    int idx = 0;
    while (g_simRun) {
        this_thread::sleep_for(chrono::seconds(2));
        if (!g_simRun) break;
        IrRawFrame frame = buildNecFrame(SIM_KEYS[idx % size(SIM_KEYS)]);
        idx++;
        g_callback(frame);
    }
}

void initIR(IrCallback cb)
{
    g_callback = cb;
    g_simRun = true;
    g_simThread = thread(simThread);
    cout << "[IR] Simulation on. A key every 2 seconds." << endl;
}

void cleanupIR()
{
    g_simRun = false;
    if (g_simThread.joinable()) g_simThread.join();
    cout << "[IR] Simulation stopped." << endl;
}

#endif
