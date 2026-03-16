#include "ir_decoder.h"
#include <iostream>

using namespace std;

// Controlla se val � entro la tolleranza di target
static bool near(uint32_t val, uint32_t target)
{
    return val >= (target - NEC_TOLERANCE) && val <= (target + NEC_TOLERANCE);
}

NecFrame decodeNEC(const IrRawFrame& raw)
{
    NecFrame out{0, 0, false, false};

    if (raw.count < 2) return out;

    int i = 0;

    // Verifica leader burst (~9ms)
    if (!near(raw.edges[i].duration_us, NEC_LEADER_BURST)) {
        cout << "[Decoder] Leader burst non valido: "
             << raw.edges[i].duration_us << "us" << endl;
        return out;
    }
    i++;

    // Leader space: 4.5ms = frame completo, 2.25ms = repeat
    if (near(raw.edges[i].duration_us, NEC_REPEAT_SPACE)) {
        out.isRepeat = true;
        out.valid = true;
        return out;
    }
    if (!near(raw.edges[i].duration_us, NEC_LEADER_SPACE)) {
        cout << "[Decoder] Leader space non valido: "
             << raw.edges[i].duration_us << "us" << endl;
        return out;
    }
    i++;

    // Leggi 32 bit, LSB first
    // Ogni bit = burst fisso + spazio variabile (corto=0, lungo=1)
    uint32_t data = 0;
    for (int bit = 0; bit < 32; bit++) {
        if (i + 1 >= raw.count) {
            cout << "[Decoder] Frame troppo corto al bit " << bit << endl;
            return out;
        }

        if (!near(raw.edges[i].duration_us, NEC_BIT_BURST)) {
            cout << "[Decoder] Burst non valido al bit " << bit
                 << ": " << raw.edges[i].duration_us << "us" << endl;
            return out;
        }
        i++;

        uint32_t space = raw.edges[i].duration_us;
        i++;

        data >>= 1; // NEC trasmette LSB first
        if (near(space, NEC_ONE_SPACE))
            data |= (1u << 31);  // bit 1
        else if (!near(space, NEC_ZERO_SPACE)) {
            cout << "[Decoder] Spazio non valido al bit " << bit
                 << ": " << space << "us" << endl;
            return out;
        }
        // bit 0: gi� 0 dopo lo shift, niente da fare
    }

    // Estrai i 4 byte dal valore a 32 bit
    // Layout: address(8) | ~address(8) | command(8) | ~command(8)
    uint8_t addr = (data >>  0) & 0xFF;
    uint8_t addrInv = (data >>  8) & 0xFF;
    uint8_t cmd = (data >> 16) & 0xFF;
    uint8_t cmdInv = (data >> 24) & 0xFF;

    // Verifica checksum: byte XOR complemento deve dare 0xFF
    if ((uint8_t)(addr ^ addrInv) != 0xFF) {
        cout << "[Decoder] Checksum address fallito" << endl;
        return out;
    }
    if ((uint8_t)(cmd ^ cmdInv) != 0xFF) {
        cout << "[Decoder] Checksum command fallito" << endl;
        return out;
    }

    out.address = addr;
    out.command = cmd;
    out.valid   = true;
    return out;
}
