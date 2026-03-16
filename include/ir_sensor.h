#ifndef __IR_SENSOR_H__
#define __IR_SENSOR_H__

#include <cstdint>
#include <functional>
#include <array>

// Timing protocollo NEC (microsecondi)
// Il telecomando Elegoo usa NEC: ogni bit � codificato dalla durata del silenzio dopo il burst. Burst fisso ~562us.
// Silenzio corto (~562us) = bit 0, silenzio lungo (~1687us) = bit 1
#define NEC_LEADER_BURST   9000
#define NEC_LEADER_SPACE   4500
#define NEC_REPEAT_SPACE   2250
#define NEC_BIT_BURST      562
#define NEC_ONE_SPACE      1687
#define NEC_ZERO_SPACE     562
#define NEC_TOLERANCE      300   // tolleranza +/- per ogni misura
#define NEC_LEADER_TOLERANCE 1500
#define NEC_FRAME_TIMEOUT  15000 // silenzio che indica fine frame
#define IR_MAX_EDGES       128   // max edge per frame (67 bastano per NEC)

// IR sensor GPIO on Raspberry Pi 5
#ifndef SIM
    #define GPIO_CHIP "/dev/gpiochip4"
    #define IR_PIN 25                
#endif

// ?? Un singolo edge (cambio di livello del segnale GPIO) ??????????????
struct IrEdge {
    uint32_t duration_us;  // quanto � durato il livello precedente
    bool level;        // nuovo livello dopo questo edge
    IrEdge() : duration_us(0), level(false) {}
    IrEdge(uint32_t d, bool l) : duration_us(d), level(l) {}
};

// ?? Frame grezzo: sequenza di edge catturati dalla ISR ????????????????
struct IrRawFrame {
    std::array<IrEdge, IR_MAX_EDGES> edges;
    int count = 0;
};

// ?? Funzione da chiamare quando arriva un frame completo ??????????????
using IrCallback = std::function<void(const IrRawFrame&)>;

// ?? Avvia il sensore (HW reale o simulazione) e registra il callback ?
void initIR(IrCallback cb);

// ?? Ferma il sensore ??????????????????????????????????????????????????
void cleanupIR();

#endif // IR_SENSOR_H
