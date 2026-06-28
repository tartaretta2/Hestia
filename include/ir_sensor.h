#ifndef __IR_SENSOR_H__
#define __IR_SENSOR_H__


#include <cstdint>
#include <functional>
#include <array>

using namespace std;

// NEC protocol timing parameters (in microseconds)
// Elegoo remote uses NEC: each bit is encoded as a burst (562us) + space
// Short silence = 562us -> bit 0, long silence = 1687us -> bit 1
// A complete frame starts with a leader burst (9000us) + leader space (4500us), followed by 32 bits (address + command)
#define NEC_LEADER_BURST   9000 // burst that starts the frame
#define NEC_LEADER_SPACE   4500 // silence after leader burst
#define NEC_REPEAT_SPACE   2250 // space after leader for repeat frames (held key)
#define NEC_BIT_BURST      562  // burst duration for all bits
#define NEC_ONE_SPACE      1687 // long silence for bit 1
#define NEC_ZERO_SPACE     562  // short silence for bit 0
#define NEC_TOLERANCE      300   // measure tolerance in microseconds
#define NEC_LEADER_TOLERANCE 2000  // tolerance for leader burst and space (longer, to allow for noise)
#define NEC_FRAME_TIMEOUT  15000 // max time to wait for a complete frame (in microseconds)
#define IR_MAX_EDGES       128   // max number of edges in a raw frame (leader + 32 bits * 2 edges/bit = 66, plus some margin for noise)

// A single edge detected by the ISR: duration of the previous level and new level after the edge
// Corresposponds to a transition, with the duration of the previous state in microseconds
struct IrEdge {
   uint32_t duration_us; // previous level duration in microseconds
   bool level; // new level after the edge: true = HIGH, false = LOW
   IrEdge() : duration_us(0), level(false) {}
   IrEdge(uint32_t d, bool l) : duration_us(d), level(l) {}
};

// Raw frame captured by the ISR: a sequence of edges (transitions) with their duration
// The count indicates how many edges were captured
struct IrRawFrame {
   array<IrEdge, IR_MAX_EDGES> edges;
   int count = 0;
};

// Type of the callback function that the IR sensor will call when a complete frame is captured and decoded
using IrCallback = function<void(const IrRawFrame&)>;


// Initializes the IR sensor (real HW or simulation) and registers the callback
void initIR(IrCallback cb);


// Shuts down the IR sensor
void cleanupIR();


#endif