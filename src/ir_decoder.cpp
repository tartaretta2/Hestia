#include "ir_decoder.h"
#include <chrono>

using namespace std;

// debounce variables to ignore same key if within 300ms
static uint8_t lastCmd = 0xFF; // 0xFF means no key pressed
static uint64_t lastTimeMs = 0; // last time a key was pressed, in milliseconds 

// Returns the current time in milliseconds
static uint64_t nowMs()
{
    using namespace chrono;
    return duration_cast<milliseconds>(
        steady_clock::now().time_since_epoch()).count();
}

// Verify if val is within interval tolerances
static bool near(uint32_t val, uint32_t target, uint32_t tolerance = NEC_TOLERANCE)
{
    return val >= (target - tolerance) && val <= (target + tolerance);
}

NecFrame decodeNEC(const IrRawFrame& raw)
{
    NecFrame out{0, 0, false, false}; // Initialize output frame as invalid

    if (raw.count < 2) return out;

    int i = 0;

    //First edge must be a falling edge (HIGH -> LOW) for the leader burst 
    //If duration is not within the expected range
    //either the frame is corrupted or capture started in the middle -> discard it and return an invalid frame
    if (!near(raw.edges[i].duration_us, NEC_LEADER_BURST, NEC_LEADER_TOLERANCE)) {
        return out;
    }
    i++;

    // Silence after leader burst determines if it's a repeat frame or a data frame
    // Check the duration of the space after the leader burst
    // about 2250us -> frame repeat
    if (near(raw.edges[i].duration_us, NEC_REPEAT_SPACE, NEC_LEADER_TOLERANCE)) {
        out.isRepeat = true;
        out.valid = true;
        return out;
    }
    // corrupted frame -> ignore it
    if (!near(raw.edges[i].duration_us, NEC_LEADER_SPACE, NEC_LEADER_TOLERANCE)) {
        return out;
    }
    i++;

    // about 4500us -> data frame to decode
    // read 32 bits of data, each bit consists of a fixed burst (~562us) followed by a variable space
    // space ~562us  -> bit 0
    // space ~1687us -> bit 1
    uint32_t data = 0;
    for (int bit = 0; bit < 32; bit++) {
        if (i + 1 >= raw.count) return out;

        if (!near(raw.edges[i].duration_us, NEC_BIT_BURST)) return out;
        i++;

        uint32_t space = raw.edges[i].duration_us;
        i++;

        data >>= 1; // shift in the next bit (LSB first)
        if (near(space, NEC_ONE_SPACE))
            data |= (1u << 31);   // bit 1
        else if (!near(space, NEC_ZERO_SPACE))
            return out; // bit not recognized, corrupted frame
        // bit 0: do nothing, already shifted in as 0
    }

    // Extract address and command from the 32-bit data
    // Layout: address(8) | ~address(8) | command(8) | ~command(8)
    uint8_t addr = (data >> 0) & 0xFF;
    uint8_t addrInv = (data >> 8) & 0xFF;
    uint8_t cmd = (data >> 16) & 0xFF;
    uint8_t cmdInv = (data >> 24) & 0xFF;

    // Verify checksum: byte XOR complement must give 0xFF, NEC transmits each byte twice in complemented form
    if ((uint8_t)(addr ^ addrInv) != 0xFF) return out;
    if ((uint8_t)(cmd ^ cmdInv)  != 0xFF) return out;

    // Debounce: same key within 300ms -> ignore
    uint64_t now = nowMs();
    if (cmd == lastCmd && (now - lastTimeMs) < 150) {
        return out;
    }
    // Update last command and time
    lastCmd = cmd;
    lastTimeMs = now;

    // Valid frame, fill the output structure
    out.address = addr;
    out.command = cmd;
    out.valid = true;
    return out;
}