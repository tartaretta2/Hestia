#include "ir_decoder.h"
#include <chrono>

using namespace std;

// Debounce: ignore same key if within 300ms
static uint8_t s_lastCmd = 0xFF;
static uint64_t s_lastTimeMs = 0;

static uint64_t nowMs()
{
    using namespace chrono;
    return duration_cast<milliseconds>(
        steady_clock::now().time_since_epoch()).count();
}

// Verify if val is within tolerances
static bool near(uint32_t val, uint32_t target, uint32_t tolerance = NEC_TOLERANCE)
{
    return val >= (target - tolerance) && val <= (target + tolerance);
}

NecFrame decodeNEC(const IrRawFrame& raw)
{
    NecFrame out{0, 0, false, false};

    if (raw.count < 2) return out;

    int i = 0;

    // Verifica leader burst (~9ms) con tolleranza larga
    if (!near(raw.edges[i].duration_us, NEC_LEADER_BURST, NEC_LEADER_TOLERANCE)) {
        return out;
    }
    i++;

    // Leader space: 4.5ms = frame completo, 2.25ms = repeat
    if (near(raw.edges[i].duration_us, NEC_REPEAT_SPACE, NEC_LEADER_TOLERANCE)) {
        out.isRepeat = true;
        out.valid = true;
        return out;
    }
    if (!near(raw.edges[i].duration_us, NEC_LEADER_SPACE, NEC_LEADER_TOLERANCE)) {
        return out;
    }
    i++;

    // Leggi 32 bit, LSB first
    // Ogni bit = burst fisso (~562us) + spazio variabile
    //   spazio ~562us  -> bit 0
    //   spazio ~1687us -> bit 1
    uint32_t data = 0;
    for (int bit = 0; bit < 32; bit++) {
        if (i + 1 >= raw.count) return out;

        if (!near(raw.edges[i].duration_us, NEC_BIT_BURST)) return out;
        i++;

        uint32_t space = raw.edges[i].duration_us;
        i++;

        data >>= 1;
        if (near(space, NEC_ONE_SPACE))
            data |= (1u << 31);   // bit 1
        else if (!near(space, NEC_ZERO_SPACE))
            return out;           // bit non riconoscibile, frame corrotto
        // bit 0: già 0 dopo lo shift
    }

    // Estrai i 4 byte
    // Layout: address(8) | ~address(8) | command(8) | ~command(8)
    uint8_t addr = (data >> 0) & 0xFF;
    uint8_t addrInv = (data >> 8) & 0xFF;
    uint8_t cmd = (data >> 16) & 0xFF;
    uint8_t cmdInv = (data >> 24) & 0xFF;

    // Verifica checksum: byte XOR complemento deve dare 0xFF
    if ((uint8_t)(addr ^ addrInv) != 0xFF) return out;
    if ((uint8_t)(cmd ^ cmdInv)  != 0xFF) return out;

    // Debounce: stesso tasto entro 300ms -> ignora
    uint64_t now = nowMs();
    if (cmd == s_lastCmd && (now - s_lastTimeMs) < 300) {
        return out;
    }
    s_lastCmd = cmd;
    s_lastTimeMs = now;

    out.address = addr;
    out.command = cmd;
    out.valid = true;
    return out;
}