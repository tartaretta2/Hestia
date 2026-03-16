#include "ir_sensor.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

#ifndef SIM
    #include <lgpio.h>
#endif

using namespace std;

// Stato interno
static IrCallback g_callback;
static mutex g_mutex;

// HW DEPENDENT SECTION
#ifndef SIM

static int g_chip = -1;
static uint64_t g_lastTick = 0;
static bool g_receiving = false;
static IrRawFrame g_buf[2];
static int g_active = 0;
static thread g_timeoutThread;
static atomic<bool> g_threadRun = false;

// Ritorna il tempo corrente in microsecondi
static uint64_t nowUs()
{
    using namespace chrono;
    return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
}

// Thread che rileva la fine del frame (silenzio > NEC_FRAME_TIMEOUT us)
// lgpio non ha watchdog built-in, quindi controlliamo ogni 1ms
static void timeoutThread()
{
    while (g_threadRun) {
        this_thread::sleep_for(chrono::milliseconds(1));
        lock_guard<mutex> lock(g_mutex);
        if (g_receiving && (nowUs() - g_lastTick) >= NEC_FRAME_TIMEOUT) {
            if (g_buf[g_active].count > 0) {
                g_callback(g_buf[g_active]);   // passa il frame al decoder
                g_active = 1 - g_active;       // svuota l'altro buffer
                g_buf[g_active] = IrRawFrame{};
            }
            g_receiving = false;
        }
    }
}

// Callback lgpio: chiamata su ogni fronte del pin GPIO
// timestamp in nanosecondi, level = nuovo livello (0=LOW burst, 1=HIGH idle)
static void edgeCallback(int /*chip*/, lgGpioAlert_p alert, void* /*userdata*/)
{
    uint64_t tickUs = alert->report.timestamp / 1000ULL;
    bool level = alert->report.level;

    lock_guard<mutex> lock(g_mutex);

    if (!g_receiving) {
        // Primo edge: memorizza solo il timestamp di partenza
        g_lastTick = tickUs;
        g_receiving = true;
        return;
    }

    // Edge successivi: calcola durata dell'impulso appena finito
    IrRawFrame& frame = g_buf[g_active];
    if (frame.count < IR_MAX_EDGES) {
        frame.edges[frame.count] = IrEdge(
            static_cast<uint32_t>(tickUs - g_lastTick), level);
        frame.count++;
    }
    g_lastTick = tickUs;
}

void initIR(IrCallback cb)
{
    g_callback = cb;

    // Apri /dev/gpiochip4 (RPi 5)
    g_chip = lgGpiochipOpen(4);
    if (g_chip < 0) {
        cerr << "[IR] Errore apertura " << GPIO_CHIP << ": "
             << lguErrorText(g_chip) << endl;
        return;
    }

    // Input con pull-up: VS1838B � open-drain (idle=HIGH, burst=LOW)
    int rc = lgGpioClaimInput(g_chip, LG_SET_PULL_UP, IR_PIN);
    if (rc < 0) {
        cerr << "[IR] lgGpioClaimInput fallito: " << lguErrorText(rc) << endl;
        lgGpiochipClose(g_chip);
        return;
    }

    // Registra callback su entrambi i fronti, debounce=0 (precisione us)
    rc = lgGpioSetAlertsFunc(g_chip, IR_PIN, edgeCallback, nullptr);
    if (rc < 0) {
        cerr << "[IR] lgGpioSetAlertsFunc fallito: " << lguErrorText(rc) << endl;
        lgGpiochipClose(g_chip);
        return;
    }
    lgGpioSetDebounce(g_chip, IR_PIN, 0);

    // Avvia il thread di rilevamento fine-frame
    g_threadRun    = true;
    g_timeoutThread = thread(timeoutThread);

    cout << "[IR] Hardware pronto su GPIO " << IR_PIN << endl;
}

void cleanupIR()
{
    g_threadRun = false;
    if (g_timeoutThread.joinable()) g_timeoutThread.join();
    lgGpioFree(g_chip, IR_PIN);
    lgGpiochipClose(g_chip);
    cout << "[IR] Hardware rilasciato." << endl;
}

// HW INDEPENDENT SECTION
// Compilata solo con -DSIM
#else

static thread g_simThread;
static atomic<bool> g_simRun = false;

// Costruisce un frame NEC sintetico dato un command byte.
// Produce la stessa identica sequenza di edge del telecomando fisico,
// cos� il decoder non sa che � simulato.
static IrRawFrame buildNecFrame(uint8_t cmd)
{
    IrRawFrame frame;
    uint8_t addr = 0x00;
    uint8_t addrInv = static_cast<uint8_t>(~addr);
    uint8_t cmdInv = static_cast<uint8_t>(~cmd);

    // 32 bit: addr(8) | ~addr(8) | cmd(8) | ~cmd(8), LSB first
    uint32_t data = (static_cast<uint32_t>(cmdInv) << 24) |
                    (static_cast<uint32_t>(cmd) << 16) |
                    (static_cast<uint32_t>(addrInv) <<  8) |
                    (static_cast<uint32_t>(addr));

    int i = 0;
    frame.edges[i++] = IrEdge(NEC_LEADER_BURST, true);   // leader burst
    frame.edges[i++] = IrEdge(NEC_LEADER_SPACE, false);  // leader space

    for (int bit = 0; bit < 32; bit++) {
        uint32_t space = (data & (1u << bit)) ? NEC_ONE_SPACE : NEC_ZERO_SPACE;
        frame.edges[i++] = IrEdge(NEC_BIT_BURST, true);  // burst fisso
        frame.edges[i++] = IrEdge(space, false); // spazio variabile
    }
    frame.edges[i++] = IrEdge(NEC_BIT_BURST, true);  // stop burst
    frame.count = i;
    return frame;
}

// Codici dei tasti Elegoo da simulare in sequenza
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
        lock_guard<mutex> lock(g_mutex);
        g_callback(frame);
    }
}

void initIR(IrCallback cb)
{
    g_callback = cb;
    g_simRun = true;
    g_simThread = thread(simThread);
    cout << "[IR] Simulazione attiva. Un tasto ogni 2 secondi." << endl;
}

void cleanupIR()
{
    g_simRun = false;
    if (g_simThread.joinable()) g_simThread.join();
    cout << "[IR] Simulazione fermata." << endl;
}

#endif
