#ifndef __IR_REMOTE_H__
#define __IR_REMOTE_H__

#include <cstdint>

// NEC codes for Elegoo remote
#define KEY_POWER       0x45
#define KEY_VOL_UP      0x46
#define KEY_FUNC_STOP   0x47
#define KEY_REWIND      0x44
#define KEY_PLAY_PAUSE  0x40
#define KEY_FAST_FWD    0x43
#define KEY_DOWN        0x07
#define KEY_VOL_DOWN    0x15
#define KEY_UP          0x09
#define KEY_EQ          0x19
#define KEY_ST_REPT     0x0D
#define KEY_0           0x16
#define KEY_1           0x0C
#define KEY_2           0x18
#define KEY_3           0x5E
#define KEY_4           0x08
#define KEY_5           0x1C
#define KEY_6           0x5A
#define KEY_7           0x42
#define KEY_8           0x52
#define KEY_9           0x4A

using namespace std;

// Smart house function controlled by the remote
enum class Action {
    None,
    AlarmToggle,     // PLAY/PAUSE -> alarm on/off
    LightsToggle,    // 1     -> lights on/off
    GateToggle,      // 2     -> gate open/close
    ToggleLightsMode, // FUNC/STOP -> toggle lights mode (manual / motion sensor)
    ToggleACMode,    // 3     -> toggle AC mode (manual / automatic)
    ToggleAC,        // 4     -> toggle AC on/off (only in manual mode
    ToggleHeatingMode,    // 0     -> toggle Heating mode (manual / automatic)
    ToggleHeating,        // 5     -> toggle Heating on/off (only in manual mode
    ShutdownSystem,   // POWER -> shutdown system
    Unknown
};

// Key description
struct Key {
    uint8_t code;
    const char* name;
    Action action;
};

// Search fot a key in among the codes, if none is found, nullptr is returned
const Key* lookupKey(uint8_t code);

// Return the name of the action
const char* actionName(Action a);

// Handle the action and print the infos about a key
void handleKey(uint8_t code);

#endif