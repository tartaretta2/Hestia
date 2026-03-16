#ifndef __IR_DECODER_H__
#define __IR_DECODER_H__

#include "ir_sensor.h"
#include <cstdint>

// Frame NEC decodificato
struct NecFrame {
    uint8_t address;    // indirizzo dispositivo (8 bit)
    uint8_t command;    // codice tasto (8 bit)
    bool isRepeat;      // true = tasto tenuto premuto
    bool valid;         // false = frame corrotto, da ignorare
};

// Decodifica un frame grezzo in NecFrame
NecFrame decodeNEC(const IrRawFrame& raw);

#endif
