#pragma once

#include "ir_sensor.h"
#include <cstdint>

using namespace std;

// NEC frame decoded
struct NecFrame {
    uint8_t address;    // device address (8 bit)
    uint8_t command;    // key code (8 bit)
    bool isRepeat;      // true = key being held
    bool valid;         // false = corrupted frame, ignored
};

// Decode raw frame into NecFrame
NecFrame decodeNEC(const IrRawFrame& raw);